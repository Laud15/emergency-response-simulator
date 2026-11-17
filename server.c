#include "file_header/macro.h"
#include "file_header/strutture.h"
#include "file_header/parse_rescuers.h"
#include "file_header/parse_emergency_types.h"
#include "file_header/parse_env.h"
#include "file_header/handler_emergency.h"
#include "file_header/write_log.h"

atomic_int go=1;
int operation_id=0;

cnd_t new_idle_pri2;
cnd_t new_idle_pri1;
cnd_t new_idle_pri0;
mtx_t mtx_call_rescuers;

rescuer_pool_t* rescuer_pools=NULL;
int rescuer_pool_count;

emergency_type_t* emergency_types=NULL;
int emergency_type_count;

mqd_t mq;
environment_t environment;

int main(){

    struct mq_attr attr;
    thrd_t threads[NUM_THREAD_SERVER];
    
    // Load grid settings, responder pools and emergency requirements.
    operation_id++;
    parser_result_t env_res = parse_env(&environment);

    if(env_res.cnt !=3 || env_res.error == SCALL_ERROR){
        perror("errore nel parsing dell'ambiente");
        exit(EXIT_FAILURE);
    }
    
    operation_id++;
    parser_result_t rescuers_res = parse_rescuers(&rescuer_pools);

    if(rescuers_res.error==SCALL_ERROR){
        perror("errore nel parsing dei rescues");
        FREE_ALL_RESCUE(&rescuer_pools, rescuers_res.cnt);
        exit(EXIT_FAILURE);
    }
    rescuer_pool_count = rescuers_res.cnt;

    operation_id++;
    parser_result_t emergency_t_res = parse_emergency_types(&emergency_types, rescuer_pools, rescuers_res.cnt);
    
    if(emergency_t_res.error==SCALL_ERROR){
        perror("errore nel parsing delle emergency_type");
        FREE_ALL_RESCUE(&rescuer_pools, rescuers_res.cnt);
        FREE_ALL_EMERGENCY(&emergency_types, emergency_t_res.cnt);
        exit(EXIT_FAILURE);
    }
    emergency_type_count=emergency_t_res.cnt;

    operation_id++;
    int id_mq_server = operation_id;

    // Create a blocking queue carrying fixed-size emergency requests.
    attr.mq_flags=0;
    attr.mq_maxmsg=10;
    attr.mq_msgsize=sizeof(emergency_request_t);
    attr.mq_curmsgs=0;
    
    // ENOENT means there is no previous queue to remove.
    if(mq_unlink(environment.queue_name)==SCALL_ERROR && errno != ENOENT){
        perror("errore nell'unlink iniziale della message queue");
        FREE_ALL_RESCUE(&rescuer_pools, rescuers_res.cnt);
        FREE_ALL_EMERGENCY(&emergency_types, emergency_t_res.cnt);
        exit(EXIT_FAILURE);
    }
    
    mq=mq_open(environment.queue_name,O_CREAT | O_RDONLY, 0620,&attr);
    
    if(mq==SCALL_ERROR){
        perror("errore nell'apertura della message queue");
        FREE_ALL_RESCUE(&rescuer_pools, rescuers_res.cnt);
        FREE_ALL_EMERGENCY(&emergency_types, emergency_t_res.cnt);
        exit(EXIT_FAILURE);
    }
    printf("Message queue %s opened successfully\n", environment.queue_name);
    write_log(time(NULL), id_mq_server, MESSAGE_QUEUE, "apertura mq lato server");

    THRD_CHECK(mtx_init(&mtx_call_rescuers, mtx_plain), "errore nella ini di mtx_call_rescuers");
    
    THRD_CHECK(cnd_init(&new_idle_pri0), "errore nella init di new_idle");
    THRD_CHECK(cnd_init(&new_idle_pri1), "errore nella init di new_idle");
    THRD_CHECK(cnd_init(&new_idle_pri2), "errore nella init di new_idle");

    printf("Starting emergency handling\n");
    // Start a new batch only after every handler in the current batch finishes.
    do{
    
        for(size_t i=0; i<NUM_THREAD_SERVER;i++){
            operation_id++;

            int* id_thrd;
            SNCALL(id_thrd, malloc(sizeof(int)), "errore nell'allocazione di memoria per l'id del thread");
            *id_thrd=operation_id;
            THRD_CHECK(thrd_create(&threads[i], handler_emergency, id_thrd), "errore nella thrd_create del server");
        }
    
        for(size_t i=0; i<NUM_THREAD_SERVER;i++){

            if(thrd_join(threads[i], NULL)==thrd_error){
                write_log(time(NULL), id_mq_server, SHUTDOWN, "chiusura server a causa di thrd_error");
                fprintf(stderr, "Error joining server handler");
                go=0;
            }
        }

    }while(go);

    FREE_ALL_RESCUE(&rescuer_pools, rescuers_res.cnt);
    FREE_ALL_EMERGENCY(&emergency_types, emergency_t_res.cnt);
    printf("\nConfiguration memory released\n");

    mtx_destroy(&mtx_call_rescuers);
   
    cnd_destroy(&new_idle_pri0);
    cnd_destroy(&new_idle_pri1);
    cnd_destroy(&new_idle_pri2);

    EQ_ERROR(mq_close(mq),"errore nella chiusura della mq");
    EQ_ERROR(mq_unlink(environment.queue_name),"errore durante l'unlink della mq");
    write_log(time(NULL), id_mq_server, MESSAGE_QUEUE, "chisura e unlink della mq");
    
    write_log(time(NULL), -1, SHUTDOWN, "chiusura server avvenuta con successo");
    printf("Server stopped\n");
    return 0;
}   

