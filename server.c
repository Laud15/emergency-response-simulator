#include "file_header/macro.h"
#include "file_header/strutture.h"
#include "file_header/parse_rescuers.h"
#include "file_header/parse_emergency_types.h"
#include "file_header/parse_env.h"
#include "file_header/handler_emergency.h"
#include "file_header/write_log.h"

atomic_int go=1;
int id_operazione=0;

cnd_t new_idle_pri2;
cnd_t new_idle_pri1;
cnd_t new_idle_pri0;
mtx_t mtx_call_rescuers;

coppie_numero_gemelli* lista_rescuers=NULL;
int len_lst_rescuers;

emergency_type_t* lista_emergency_type=NULL;
int len_lst_emergency_t;

mqd_t mq;
env ambiente;

//in caso ci siano problemi con la lettura dei file usare l'istruzione bash "sed -i '${/^$/d;}' file_directory" per cancellare possibili righe vuote nel file
int main(){

    struct mq_attr attr;
    thrd_t threads[NUM_THREAD_SERVER];
    
    //faccio il parsing dell'ambiente
    id_operazione++;
    res_parser env_res = parse_env(&ambiente);

    if(env_res.cnt !=3 || env_res.error == SCALL_ERROR){
        perror("errore nel parsing dell'ambiente");//uso perror coì so se l'errore è dettato da una delle chiamate di librerie o da un errore di lettura
        exit(EXIT_FAILURE);
    }
    //finisco il parsing dell'ambiente
    
    //faccio il parsing dei tipi di soccorritori
    id_operazione++;
    res_parser rescuers_res = parse_rescuers(&lista_rescuers);

    if(rescuers_res.error==SCALL_ERROR){
        perror("errore nel parsing dei rescues");
        FREE_ALL_RESCUE(&lista_rescuers, rescuers_res.cnt);
        exit(EXIT_FAILURE);
    }
    len_lst_rescuers = rescuers_res.cnt;
    //finisco il parsing dei tipi di soccorritori

    //faccio il parsing dei tipi di emergenze
    id_operazione++;
    res_parser emergency_t_res = parse_emergency_types(&lista_emergency_type, lista_rescuers, rescuers_res.cnt);
    
    if(emergency_t_res.error==SCALL_ERROR){
        perror("errore nel parsing delle emergency_type");
        FREE_ALL_RESCUE(&lista_rescuers, rescuers_res.cnt);
        FREE_ALL_EMERGENCY(&lista_emergency_type, emergency_t_res.cnt);
        exit(EXIT_FAILURE);
    }
    len_lst_emergency_t=emergency_t_res.cnt;
    //finisco il parsing dei tipi di emergenze

    id_operazione++;
    int id_mq_server = id_operazione;

    //PARTE APERTURA MESSAGE QUEUE
    attr.mq_flags=0;//faccio lavorare la coda in modalità bloccante
    attr.mq_maxmsg=10;//setto il numero massimo di messaggi
    attr.mq_msgsize=sizeof(emergency_request_t);//ogni messaggio è della dimensione di request
    attr.mq_curmsgs=0;
    
    if(mq_unlink(ambiente.nome_msg_queue)==SCALL_ERROR && errno != ENOENT){//visto che unlink potrebbe fallire perchè non c'è nessuna mq controllo che errno sia diverso da ENOENT
        perror("errore nell'unlink iniziale della message queue");
        FREE_ALL_RESCUE(&lista_rescuers, rescuers_res.cnt);
        FREE_ALL_EMERGENCY(&lista_emergency_type, emergency_t_res.cnt);
        exit(EXIT_FAILURE);
    }
    
    mq=mq_open(ambiente.nome_msg_queue,O_CREAT | O_RDONLY, 0620,&attr);//apertura della mq
    
    if(mq==SCALL_ERROR){
        perror("errore nell'apertura della message queue");
        FREE_ALL_RESCUE(&lista_rescuers, rescuers_res.cnt);
        FREE_ALL_EMERGENCY(&lista_emergency_type, emergency_t_res.cnt);
        exit(EXIT_FAILURE);
    }
    printf("message queue %s aperta con successo\n", ambiente.nome_msg_queue);
    write_log(time(NULL), id_mq_server, MESSAGE_QUEUE, "apertura mq lato server");
    

    THRD_CHECK(mtx_init(&mtx_call_rescuers, mtx_plain), "errore nella ini di mtx_call_rescuers");
    
    THRD_CHECK(cnd_init(&new_idle_pri0), "errore nella init di new_idle");
    THRD_CHECK(cnd_init(&new_idle_pri1), "errore nella init di new_idle");
    THRD_CHECK(cnd_init(&new_idle_pri2), "errore nella init di new_idle");
    //FINE APERTURA MQ

    printf("INIZIO GESTIONE EMERGENZE\n");
    //PARTE GENERAZIONE(E JOIN) DEI THREAD
    do{
    
        for(size_t i=0; i<NUM_THREAD_SERVER;i++){
            id_operazione++;

            int* id_thrd;
            SNCALL(id_thrd, malloc(sizeof(int)), "errore nell'allocazione di memoria per l'id del thread");
            *id_thrd=id_operazione;
            THRD_CHECK(thrd_create(&threads[i], handler_emergency, id_thrd), "errore nella thrd_create del server");
        }
    
        for(size_t i=0; i<NUM_THREAD_SERVER;i++){

            if(thrd_join(threads[i], NULL)==thrd_error){
                write_log(time(NULL), id_mq_server, SHUTDOWN, "chiusura server a causa di thrd_error");
                fprintf(stderr, "errore durante la thrd_join");
                go=0;
            }
        }

    }while(go);

    //INIZIO ROUTINE DI CHIUSURA
    FREE_ALL_RESCUE(&lista_rescuers, rescuers_res.cnt);
    FREE_ALL_EMERGENCY(&lista_emergency_type, emergency_t_res.cnt);
    printf("\ndeallocazione della memoria avvenuta con successo\n");

    mtx_destroy(&mtx_call_rescuers);
   
    cnd_destroy(&new_idle_pri0);
    cnd_destroy(&new_idle_pri1);
    cnd_destroy(&new_idle_pri2);

    EQ_ERROR(mq_close(mq),"errore nella chiusura della mq");
    EQ_ERROR(mq_unlink(ambiente.nome_msg_queue),"errore durante l'unlink della mq");
    write_log(time(NULL), id_mq_server, MESSAGE_QUEUE, "chisura e unlink della mq");
    //FINE ROUTINE  DI CHIUSURA 
    
    write_log(time(NULL), -1, SHUTDOWN, "chiusura server avvenuta con successo");
    printf("server chiuso!\n");
    return 0;
}   

