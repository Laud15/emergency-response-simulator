#include "../file_header/thread_client.h"
#include "../file_header/write_log.h"
  

int thread_parse_function(void* arg){

    data_client_parse* data = (data_client_parse *) arg; //assegno arg a data e lo casto come data_client_parse*

    emergency_request_t emergenza;
    short prio;
    int (*f)(char*, emergency_request_t*) = data->f;//prelevo la funzione di parse
    int delay = f(data->riga, &emergenza);//prendo il delay restituito da  f(riga, &emergenza) popolando così emergenza

    if((strcmp(emergenza.emergency_name, "Errore"))==0){//se una delle linee è stata invalidata non la mando nella msgq
        write_log(time(NULL), 0, INVALID_INPUT, "formato della stringa non valido");
        printf("formato non valido\n");
        return thrd_success;
    }

    for(size_t i=0;i<data->len_em_pri;i++){

        if(strcmp(emergenza.emergency_name, data->list_em_pri[i].nome)==0){//controllo se l'emergenza esiste
            prio=data->list_em_pri[i].prio;//se esiste prendo la sua priorità
            break;
        }else if(i==(data->len_em_pri-1)){
            prio=0;
            strcpy(emergenza.emergency_name, "NotFound");
        }

    }
    
    if(delay>=0){//controllo che il delay sia un valore positivo
        sleep(delay);//dormo per i secondi specificati
    }else{

        sleep(0);//se non lo è lo faccio addormentare per 0 secondi
    }

    emergenza.timestamp=time(NULL);//setto il timestamp

    EQ_ERROR(mq_send(data->mq, (char*)(&emergenza), sizeof(emergency_request_t), prio), "errore nell'invio del messaggio");
    write_log(time(NULL), 0, CLIENT_REQUEST, data->riga);
    printf("messaggio inviato con successo da parte di -> %s <x:%d, y:%d> con priorità %hd\n", emergenza.emergency_name, emergenza.x, emergenza.y, prio);

    free(data->riga);
    free(data);
    return thrd_success;
}
 