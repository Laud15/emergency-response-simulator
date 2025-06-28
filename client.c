#include "file_header/macro.h"
#include "file_header/strutture.h"
#include "file_header/parse_env.h"
#include "file_header/parse_request.h"
#include "file_header/write_log.h"

int main(int argc, char* argv[]){

    mqd_t mq;//dichiaro la mq
    
    if(argc > 1){


        //INIZIO PARTE SHM________________________________________________________________________________________________
        char nome_mq[QUEUE_NAME_LENGTH];
        coppia_emergenza_priorità* coppie_em_pri_client=NULL;
        coppia_emergenza_priorità* dati_shm=NULL;
        int fd_shm;

        SCALL(fd_shm, shm_open(SHM_NAME, O_RDONLY, 0620), "errore nell'apertura della shm");
        write_log(time(NULL), 0, SHM, "apertura shm lato client");

        MMAP(dati_shm, mmap(NULL, sizeof(coppia_emergenza_priorità), PROT_READ, MAP_SHARED, fd_shm, 0), "errore durante la 1° mmap");

        int len_em_pri = dati_shm->prio;//salvo la lunghezza della lista in una variabile
        strcpy(nome_mq, dati_shm->nome);//prendo il nome della mq

        MMAP(dati_shm, mmap(NULL, (sizeof(coppia_emergenza_priorità)*len_em_pri), PROT_READ, MAP_SHARED, fd_shm, 0), "errore durante la 2° mmap");
        
        SNCALL(coppie_em_pri_client, malloc(sizeof(coppia_emergenza_priorità)*len_em_pri), "errore durante la malloc delle coppie per il client");
        
        for(size_t i=1;i<(len_em_pri+1);i++){
            coppie_em_pri_client[i-1]=dati_shm[i];//copio gli elementi della lista trovata nella shm nella lista che userò nel resto del programma
        }
        write_log(time(NULL), 0, SHM, "letta lista coppie_em_pri");

        EQ_ERROR(munmap(dati_shm, (sizeof(coppia_emergenza_priorità)*len_em_pri)), "errore nella munmap");
        EQ_ERROR(close(fd_shm),"errore nella chiusura della shm");//libero la memoria e chiudo la shm
        write_log(time(NULL), 0, SHM, "chiusura shm lato client");

        //FINE PARTE SHM_____________________________________________________________________


        SCALL(mq, mq_open(nome_mq ,O_WRONLY), "errore nell'apertura della mq");//controllo che l'apertura della mq vada a buon fine
        write_log(time(NULL), 0, MESSAGE_QUEUE, "message queue lato server aperta");  

        /*for(size_t i=0;i<len_em_pri;i++){
            printf("nome=%s, prio=%hd\n", coppie_em_pri_client[i].nome, coppie_em_pri_client[i].prio);
        }*/

        if(argc==3 && (strcmp(argv[1], "-f"))==0){//CONTROLLO SE HO IL FILE

            res_parser res_scroll_file;//dichiaro la variabile che accoglierà il risultato del parse
            res_scroll_file=parse_file(argv[2], mq, coppie_em_pri_client, len_em_pri);//parso il file e invio il risultato nella mq (il parse di ogni riga e l'invio è gestito da più thread)
            
            if(res_scroll_file.error==SCALL_ERROR){//controllo che non ci siano stato errori durante il parsing e l'invio 

                EQ_ERROR(mq_close(mq),"errore nella chiusura della mq");
                perror("errore nel parsing del file");
                exit(EXIT_FAILURE);

            }
            
        }else if(argc == 5){//CASO IN CUI HO SOLO UN FILE
            
            char buffer[BUFFERSIZE];
            emergency_request_t emergenza;//dichiaro l'emergenza e poi assegno i tipi letti da argv
            strcpy(emergenza.emergency_name, argv[1]);
            emergenza.x=atoi(argv[2]);
            emergenza.y=atoi(argv[3]);
            int delay = atoi(argv[4]);
            int prio;

            for(size_t i=0; i<len_em_pri;i++){
                if(strcmp(emergenza.emergency_name, coppie_em_pri_client[i].nome)==0){
                    prio=coppie_em_pri_client[i].prio;
                    break;
                }else if(i==(len_em_pri-1)){
                    prio=0;
                    strcpy(emergenza.emergency_name, "NotFound");
                }
            }

            if(delay>=0){//controllo che il delay sia positivo
                sleep(delay);
            }else{
                sleep(0);//se non lo è lo interpreto come sleep(0)
            }

            emergenza.timestamp=time(NULL);
            
            EQ_ERROR(mq_send(mq, (char*)(&emergenza), sizeof(emergenza), prio), "errore nell'invio del messaggio");

            snprintf(buffer, BUFFERSIZE, "%s %d %d %d", emergenza.emergency_name, emergenza.x, emergenza.y, delay);
            write_log(time(NULL), 0, CLIENT_REQUEST, buffer);
            printf("messaggio inviato con successo\n");

        }else if(strcmp(argv[1], "shutdown_server")==0){

            emergency_request_t sd_request;
            strcpy(sd_request.emergency_name, argv[1]);
            sd_request.timestamp=time(NULL);
            sd_request.x=-1;
            sd_request.x=-1;

            for(size_t i=0;i<NUM_THREAD_SERVER;i++){
                EQ_ERROR(mq_send(mq, (char*)(&sd_request), sizeof(sd_request), 3), "errore nell'invio della richiesta di shutdown");
            }

            write_log(time(NULL), 0, CLIENT_REQUEST, "shutdown_server richiesto da client");

        }else{//CASO IN CUI NON HO -f E NEANCHE GLI ELEMENTI GIUSTI
            printf("riga non valida passata!\n");
            write_log(time(NULL), 0, INVALID_INPUT, "input non valido dato da riga di comando");
        }
    
    free(coppie_em_pri_client);
    EQ_ERROR(mq_close(mq),"errore nella chiusura della mq");
    write_log(time(NULL), 0, MESSAGE_QUEUE, "mq lato client chiusa");

    }else{fprintf(stderr,"non sono stati passati argomenti");}

    return 0;
}

