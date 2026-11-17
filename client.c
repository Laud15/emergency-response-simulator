#include "file_header/macro.h"
#include "file_header/strutture.h"
#include "file_header/parse_env.h"
#include "file_header/parse_request.h"
#include "file_header/write_log.h"

int main(int argc, char* argv[]){

    mqd_t mq;

    if(argc > 1){

        parser_result_t env_res;
        environment_t environment;
        env_res=parse_env(&environment);

        if(env_res.error==SCALL_ERROR){
            perror("errore nel parse dell'ambiente");
            exit(EXIT_FAILURE);
        }
        
        SCALL(mq, mq_open(environment.queue_name ,O_WRONLY), "errore nell'apertura della mq");
        write_log(time(NULL), 0, MESSAGE_QUEUE, "message queue lato server aperta");  

        if(argc==3 && (strcmp(argv[1], "-f"))==STRCMP_TRUE){

            parser_result_t res_file;
            // Parse and send file requests sequentially, applying each line's delay.
            res_file=parse_and_send(argv[2], mq);
            
            if(res_file.error==SCALL_ERROR){

                EQ_ERROR(mq_close(mq),"errore nella chiusura della mq");
                perror("errore nel parsing del file");
                exit(EXIT_FAILURE);

            }
            
        }else if(argc == 5){
            // Submit one request from command-line arguments.
            
            char buffer[BUFFERSIZE];
            emergency_request_t emergency;
            strcpy(emergency.emergency_name, argv[1]);
            emergency.x=atoi(argv[2]);
            emergency.y=atoi(argv[3]);
            int delay = atoi(argv[4]);

            // Command-line mode treats negative delays as zero.
            if(delay>=0){
                sleep(delay);
            }else{
                sleep(0);
            }

            emergency.timestamp=time(NULL);
            
            EQ_ERROR(mq_send(mq, (char*)(&emergency), sizeof(emergency), 0), "errore nell'invio del messaggio");

            snprintf(buffer, BUFFERSIZE, "%s %d %d %d", emergency.emergency_name, emergency.x, emergency.y, delay);
            write_log(time(NULL), 0, CLIENT_REQUEST, buffer);
            printf("Request sent successfully\n");

        }else if(strcmp(argv[1], "shutdown_server")==STRCMP_TRUE){

            emergency_request_t sd_request;
            strcpy(sd_request.emergency_name, argv[1]);
            sd_request.timestamp=time(NULL);
            sd_request.x=-1;
            sd_request.x=-1;

            // Send one higher-priority shutdown message per server handler.
            for(size_t i=0;i<NUM_THREAD_SERVER;i++){
                EQ_ERROR(mq_send(mq, (char*)(&sd_request), sizeof(sd_request), 3), "errore nell'invio della richiesta di shutdown");
            }

            write_log(time(NULL), 0, CLIENT_REQUEST, "shutdown_server richiesto da client");

        }else{
            printf("Invalid command-line arguments\n");
            write_log(time(NULL), 0, INVALID_INPUT, "input non valido dato da riga di comando");
        }
    
    EQ_ERROR(mq_close(mq),"errore nella chiusura della mq");
    write_log(time(NULL), 0, MESSAGE_QUEUE, "mq lato client chiusa");

    }else{
        fprintf(stderr,"No arguments provided");
    }

    return 0;
}
