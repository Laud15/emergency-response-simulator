#include "file_header/macro.h"
#include "file_header/strutture.h"
#include "file_header/parse_env.h"
#include "file_header/parse_request.h"
#include "file_header/write_log.h"

int main(int argc, char* argv[]){

    mqd_t mq;//dichiaro la mq
    

    if(argc > 1){

        res_parser env_res;
        env ambiente;
        env_res=parse_env(&ambiente);

        if(env_res.error==SCALL_ERROR){//controllo che il parse sia andato a buon fine
            perror("errore nel parse dell'ambiente");
            exit(EXIT_FAILURE);
        }
        
        SCALL(mq, mq_open(ambiente.nome_msg_queue ,O_WRONLY), "errore nell'apertura della mq");//controllo che l'apertura della mq vada a buon fine
        write_log(time(NULL), 0, MESSAGE_QUEUE, "message queue lato server aperta");  


        if(argc==3 && (strcmp(argv[1], "-f"))==STRCMP_TRUE){//CONTROLLO SE HO IL FILE

            res_parser res_file;//dichiaro la variabile che accoglierà il risultato del parse
            res_file=parse_and_send(argv[2], mq);//parso il file e invio il risultato nella mq (il parse di ogni riga e l'invio è gestito da più thread)
            
            if(res_file.error==SCALL_ERROR){//controllo che non ci siano stato errori durante il parsing e l'invio 

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

            if(delay>=0){//controllo che il delay sia positivo
                sleep(delay);
            }else{
                sleep(0);//se non lo è lo interpreto come sleep(0)
            }

            emergenza.timestamp=time(NULL);
            
            EQ_ERROR(mq_send(mq, (char*)(&emergenza), sizeof(emergenza), 0), "errore nell'invio del messaggio");

            snprintf(buffer, BUFFERSIZE, "%s %d %d %d", emergenza.emergency_name, emergenza.x, emergenza.y, delay);
            write_log(time(NULL), 0, CLIENT_REQUEST, buffer);
            printf("messaggio inviato con successo\n");
        
            
        }else if(strcmp(argv[1], "shutdown_server")==STRCMP_TRUE){

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
    
    EQ_ERROR(mq_close(mq),"errore nella chiusura della mq");
    write_log(time(NULL), 0, MESSAGE_QUEUE, "mq lato client chiusa");

    }else{
        fprintf(stderr,"non sono stati passati argomenti");
    }

    return 0;
}