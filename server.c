#include "file_header/macro.h"
#include "file_header/strutture.h"
#include "file_header/parse_rescuers.h"
#include "file_header/parse_emergency_types.h"
#include "file_header/parse_env.h"
#include "file_header/handler_emergency.h"
#include "file_header/write_log.h"

volatile sig_atomic_t go=1;
int id_operazione=0;

//in caso ci siano problemi con la lettura dei file usare l'istruzione bash "sed -i '${/^$/d;}' file_directory" per cancellare possibili righe vuote nel file
int main(){

    mqd_t mq;
    env ambiente;
    coppie_numero_gemelli* lista_rescuers=NULL;
    coppia_emergenza_priorità* coppie_em_prio=NULL;
    emergency_type_t* lista_emergency=NULL;
    struct mq_attr attr;
    int fd_shm, shm_size;
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
    //finisco il parsing dei tipi di soccorritori

    //faccio il parsing dei tipi di emergenze
    id_operazione++;
    res_parser emergency_t_res = parse_emergency_types(&lista_emergency,lista_rescuers, rescuers_res.cnt);

    if(emergency_t_res.error==SCALL_ERROR){
        perror("errore nel parsing delle emergency_type");
        FREE_ALL_RESCUE(&lista_rescuers, rescuers_res.cnt);
        FREE_ALL_EMERGENCY(&lista_emergency, emergency_t_res.cnt);
        exit(EXIT_FAILURE);
    }
    //finisco il parsing dei tipi di emergenze

    //STAMPA DEI RESCUER
    /*for(size_t i=0;i<rescuers_res.cnt;i++){
        for(size_t j=0;j<lista_rescuers[i].n;j++){
            printf("ciao sono %d e sono un gemello di %p\n", lista_rescuers[i].lst_gemelli[j].id, lista_rescuers[i].lst_gemelli[j].rescuer);
        }
    }*/


    //STAMPA DEI TIPI DI EMERGENZA
    /*for(size_t i=0;i<emergency_t_res.cnt;i++){
        printf("sono l'emergenza:%s e la mia priorità è: %hd\n", lista_emergency[i].emergency_desc,lista_emergency[i].priority);
        printf("tipi di rescuer richiesti:%d\n",lista_emergency[i].rescuers_req_number);
        printf("questi sono i soccoritori di cui ho bisogno:\n");
        for(size_t j=0;j<lista_emergency[i].rescuers_req_number;j++){
            printf("-nome:%s, numero richiesto:%d, tempo:%d\n", lista_emergency[i].rescuers[j].type->rescuer_type_name,lista_emergency[i].rescuers[j].required_count,lista_emergency[i].rescuers[j].time_to_manage);
        }
    }*/




    //INIZIO PARTE DI SHM _____________________________________________________________________________________________ 
    id_operazione++;
    int id_shm = id_operazione;
    if(shm_unlink(SHM_NAME)==SCALL_ERROR && errno != ENOENT){//unlinko per evitare possibili errori in caso la shm non fosse stata chiusa correttamente
        perror("errore durante la unlink di sicurezza");
        FREE_ALL_RESCUE(&lista_rescuers, rescuers_res.cnt);
        FREE_ALL_EMERGENCY(&lista_emergency, emergency_t_res.cnt);
        exit(EXIT_FAILURE);
    };

    shm_size=(sizeof(coppia_emergenza_priorità)*(emergency_t_res.cnt+1));

    fd_shm=shm_open(SHM_NAME, O_CREAT | O_RDWR, 0620);

    if(fd_shm == SCALL_ERROR){//controllo che la open vada a buon fine
        perror("errore nella shm_open");
        FREE_ALL_RESCUE(&lista_rescuers, rescuers_res.cnt);
        FREE_ALL_EMERGENCY(&lista_emergency, emergency_t_res.cnt);
        close(fd_shm);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }
    write_log(time(NULL), id_shm, SHM, "apertura shm lato server");

    if(ftruncate(fd_shm, shm_size) == SCALL_ERROR){//controllo che la ftruncate vada a buon fine
        FREE_ALL_RESCUE(&lista_rescuers, rescuers_res.cnt);
        FREE_ALL_EMERGENCY(&lista_emergency, emergency_t_res.cnt);

    }
    
    coppie_em_prio = mmap(NULL, shm_size, PROT_WRITE, MAP_SHARED, fd_shm, 0);//alloco la memoria per la lista delle priorità
    
    if(coppie_em_prio==MAP_FAILED){//controllo che la mmap vada a buon fine
        perror("errore nella mmap");
        FREE_ALL_RESCUE(&lista_rescuers, rescuers_res.cnt);
        FREE_ALL_EMERGENCY(&lista_emergency, emergency_t_res.cnt);
        munmap((coppie_em_prio), shm_size);
        close(fd_shm);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    //IL PRIMO ELEMENTO DELLA SHM NON CONTIENE UN EMERGENZA E UNA PRIORITA MA CONTIENE IL NUMERO DI EMERGENZE E IL NOME DELLA MQ
    strcpy(coppie_em_prio[0].nome, ambiente.nome_msg_queue);
    coppie_em_prio[0].prio=emergency_t_res.cnt;
    write_log(time(NULL), id_shm, SHM, "comunicata lunghezza lista emergenza");

    for(size_t i=1;i<emergency_t_res.cnt+1;i++){//inserisco gli elementi
        strcpy(coppie_em_prio[i].nome, lista_emergency[i-1].emergency_desc);
        coppie_em_prio[i].prio=lista_emergency[i-1].priority;
    }
    write_log(time(NULL), id_shm, SHM, "emergenze e priorità inserite nell shm");
    //FINE PARTE SHM_____________________________________________________________________________

    //printf("nome queue:%s, heigth:%d, width:%d\n", ambiente.nome_msg_queue, ambiente.height, ambiente.width);
    id_operazione++;
    int id_mq_server = id_operazione;

    attr.mq_flags=0;//faccio lavorare la coda in modalità bloccante
    attr.mq_maxmsg=10;//setto il numero massimo di messaggi
    attr.mq_msgsize=sizeof(emergency_request_t);//ogni messaggio è della dimensione di request
    attr.mq_curmsgs=0;

    if(mq_unlink(ambiente.nome_msg_queue)==SCALL_ERROR && errno != ENOENT){//visto che unlink potrebbe fallire perchè non c'è nessuna mq controllo che errno sia diverso da ENOENT
        perror("errore nell'unlink iniziale della message queue");
        FREE_ALL_RESCUE(&lista_rescuers, rescuers_res.cnt);
        FREE_ALL_EMERGENCY(&lista_emergency, emergency_t_res.cnt);
        exit(EXIT_FAILURE);
    }
    
    mq=mq_open(ambiente.nome_msg_queue,O_CREAT | O_RDONLY, 0620,&attr);//apertura della mq
    
    if(mq==SCALL_ERROR){
        perror("errore nell'apertura della message queue");
        FREE_ALL_RESCUE(&lista_rescuers, rescuers_res.cnt);
        FREE_ALL_EMERGENCY(&lista_emergency, emergency_t_res.cnt);
        exit(EXIT_FAILURE);
    }
    printf("message queue %s aperta con successo\n", ambiente.nome_msg_queue);
    write_log(time(NULL), id_mq_server, MESSAGE_QUEUE, "apertura mq lato server");
    

    THRD_CHECK(mtx_init(&mutex_rescuer, mtx_plain), "errore nella init di mutex_rescuer");
    THRD_CHECK(mtx_init(&mtx_call_rescuers, mtx_plain), "errore nella ini di mtx_call_rescuers");
    THRD_CHECK(cnd_init(&new_idle), "errore nella init di new_idle");


    printf("INIZIO GESTIONE EMERGENZE\n");

    do{
    
        for(size_t i=0; i<NUM_THREAD_SERVER;i++){
            id_operazione++;
            
            data_server_thread* data=malloc(sizeof(data_server_thread));
            data->id=id_operazione;
            data->amb=ambiente;
            data->lista_soccorritori=lista_rescuers;
            data->lista_tipi_emergenze=lista_emergency;
            data->len_lst_em=emergency_t_res.cnt;
            data->len_lst_socc=rescuers_res.cnt;
            data->mq=&mq;

            THRD_CHECK(thrd_create(&threads[i], handler_emergency, data), "errore nella thrd_create del server");
        }
    
        
        for(size_t i=0; i<NUM_THREAD_SERVER;i++){

            if(thrd_join(threads[i], NULL)==thrd_error){
                write_log(time(NULL), id_mq_server, SHUTDOWN, "chiusura server in corso...");
                printf("ciao\n");
                go=0;
            }
        }


    }while(go);


    FREE_ALL_RESCUE(&lista_rescuers, rescuers_res.cnt);
    FREE_ALL_EMERGENCY(&lista_emergency, emergency_t_res.cnt);
    printf("\ndeallocazione della memoria avvenuta con successo\n");

    mtx_destroy(&mutex_rescuer);
    mtx_destroy(&mtx_call_rescuers);
    cnd_destroy(&new_idle);

    EQ_ERROR(munmap((coppie_em_prio), shm_size), "errore nella munmap della shm");
    EQ_ERROR(close(fd_shm), "errore nella chiusura della shm");
    EQ_ERROR(shm_unlink(SHM_NAME), "errore nell");
    write_log(time(NULL), id_shm, SHM, "chiusura completa della shm");

    EQ_ERROR(mq_close(mq),"errore nella chiusura della mq");
    EQ_ERROR(mq_unlink(ambiente.nome_msg_queue),"errore durante l'unlink della mq");
    write_log(time(NULL), id_mq_server, MESSAGE_QUEUE, "chisura e unlink della mq");
    //printf("chiusura e unlink della mq avvenuta con successo\n");
    
    

    write_log(time(NULL), 0, SHUTDOWN, "chiusura server avvenuta con successo");
    printf("server chiuso!\n");
    return 0;
}   

