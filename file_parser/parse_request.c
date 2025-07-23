#include "../file_header/parse_request.h"
#include "../file_header/write_log.h"

int id_operazione=0;

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

res_parser parse_and_send(char* directory, mqd_t mq){

    res_parser res;//dichiaro il tipo di ritorno
    res.cnt=0;//inzializzo il campo conta
    res.error=0;//setto l'errore a 0
    FILE* fd;//inizializzo il file descriptor
    char buffer[BUFFERSIZE];//dichiaro il buffer che usero per la fgets
    int delay;//setto la dimensione della lista
    emergency_request_t emergenza;//la struttura che conterrà i dati della riga parsata
    SNCALL(fd,fopen(directory, "r"), "errore nell'apertura del file");//controllo che il venga aperto correttamente

    while (fgets(buffer, BUFFERSIZE, fd)!=NULL){//

        if(buffer[0] == '\n' || buffer[0] == '\0'){//caso in cui ci sia una riga vuota
            continue;
        }

        delay = parse_riga(buffer, &emergenza);//parso la riga e la metto nella struttura emergenza

        if(strcmp("Errore", emergenza.emergency_name)!=0){//se la riga era in un formato non corretto non invio e scrivo nel file di log

            sleep(delay);//aspetto il delay

            emergenza.timestamp=time(NULL); //assegno il tempo all'emergenza
            EQ_ERROR(mq_send(mq, (char*)(&emergenza), sizeof(emergency_request_t), 0),"errore nell'invio del messaggio nella mq");//invio nella mq la struttura contentne i dati letti

            write_log(time(NULL), 0, MESSAGE_QUEUE, buffer);//scrivo nel log

            printf("messaggio: %s inviato con successo\n", buffer);

        }else{

            write_log(time(NULL), 0, CLIENT_REQUEST, "riga letta in formato non valido");

        }

        res.cnt++;

    }

    CHECK_END_FGETS(fd, res, "lettura file e invio messaggi completato con successo\n");//controllo che la fgets sia terminata perchè arrivata a EOF
    CHECK_FCLOSE(fd,res,"errore nella chiusura del file");//controllo la chiusura corretta del file
    
    return res;
}




