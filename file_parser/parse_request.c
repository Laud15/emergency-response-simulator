#include "../file_header/parse_request.h"
#include "../file_header/thread_client.h"
#include "../file_header/write_log.h"

int parse_riga(char* riga, emergency_request_t* emergenza){
    int delay;

    if(sscanf(riga, "%127[^ ] %d %d %d", emergenza->emergency_name, &(emergenza->x), &(emergenza->y), &delay)!=4){//se leggo un numero diverso dai caratteri attesi 
        strcpy(emergenza->emergency_name, "Errore");
        emergenza->x=SCALL_ERROR;
        emergenza->y=-SCALL_ERROR;
        delay=0;
    }

    return delay;
}

res_parser parse_file(char* directory, mqd_t mq, coppia_emergenza_priorità* list_em_prio, int len_em_prio){

    res_parser res;//dichiaro il tipo di ritorno
    res.cnt=0;//inzializzo il campo conta
    res.error=0;//setto l'errore a 0
    FILE* fd;//inizializzo il file descriptor
    char buffer[BUFFERSIZE];//dichiaro il buffer che usero per la fgets
    int dim_list= START_LST_DIM;//setto la dimensione della lista
    thrd_t* threads=NULL;//inizializzo la lista che conterrà i thread
    
    SNCALL(fd,fopen(directory, "r"), "errore nell'apertura del file");//controllo che il venga aperto correttamente
    NCALL_PARSER(threads, malloc(sizeof(thrd_t)*START_LST_DIM),"errore nell'allocazione di memoria iniziale per i thread", res, fd);


    while (fgets(buffer, BUFFERSIZE, fd)!=NULL){//
       
        thrd_t* tmp;
        //controllo se c'+ bisogno di reallocare
        if(res.cnt==dim_list){
            dim_list*=2;
            NCALL_PARSER(tmp, realloc(threads,(sizeof(thrd_t)*(dim_list))), "errore nella reallocazione di memoria", res, fd);
            threads=tmp;
        }
        
        //creo la struttura da passare al thread come argomento
        data_client_parse* data;
        NCALL_PARSER(data, malloc(sizeof(data_client_parse)), "errore nell'allocazione di memoria per i dati da mandare al thread", res, fd);
        NCALL_PARSER(data->riga, malloc(sizeof(char)*strlen(buffer)), "errore nell'allocazione della riga per il thread", res, fd);
        strcpy(data->riga, buffer);
        
        data->f=parse_riga;
        data->mq=mq;
        data->list_em_pri=list_em_prio;
        data->len_em_pri=len_em_prio;

        THRD_CHECK(thrd_create(&(threads[res.cnt]), thread_parse_function, data), "errore nella thrd_create lato server");
        res.cnt++;
    }

    for(size_t i=0; i<res.cnt;i++){
        THRD_CHECK(thrd_join(threads[i], NULL), "errore nella thrd_join lato server");
    }

    free(threads);
    CHECK_END_FGETS(fd, res, "lettura file e invio messaggi completato con successo\n");//controllo che la fgets sia terminata perchè arrivata a EOF
    CHECK_FCLOSE(fd,res,"errore nella chiusura del file");//controllo la chiusura corretta del file
    
    return res;
}




