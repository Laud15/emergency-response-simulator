#include "../file_header/parse_request.h"
#include "../file_header/write_log.h"

int operation_id=0;

int parse_request_line(char* line, emergency_request_t* emergency){

    int delay;

    // Mark malformed rows with the sentinel name checked by parse_and_send.
    if(sscanf(line, "%127[^ ] %d %d %d", emergency->emergency_name, &(emergency->x), &(emergency->y), &delay)!=4){
        strcpy(emergency->emergency_name, "Errore");
        emergency->x=SCALL_ERROR;
        emergency->y=-SCALL_ERROR;
        delay=0;
    }

    return delay;
}

parser_result_t parse_and_send(char* directory, mqd_t mq){

    parser_result_t res;
    res.cnt=0;
    res.error=0;
    FILE* fd;
    char buffer[BUFFERSIZE];
    int delay;
    emergency_request_t emergency;
    SNCALL(fd,fopen(directory, "r"), "errore nell'apertura del file");

    // Read and send sequentially; blank lines do not introduce a delay.
    while (fgets(buffer, BUFFERSIZE, fd)!=NULL){

        if(buffer[0] == '\n' || buffer[0] == '\0'){
            continue;
        }

        delay = parse_request_line(buffer, &emergency);

        if(strcmp("Errore", emergency.emergency_name)!=0){

            // File mode passes the parsed delay directly to sleep; use nonnegative values.
            sleep(delay);

            // Timestamp the request after its delay, immediately before sending.
            emergency.timestamp=time(NULL);
            EQ_ERROR(mq_send(mq, (char*)(&emergency), sizeof(emergency_request_t), 0),"errore nell'invio del messaggio nella mq");

            write_log(time(NULL), 0, MESSAGE_QUEUE, buffer);

            printf("Request: %s sent successfully\n", buffer);

        }else{

            write_log(time(NULL), 0, CLIENT_REQUEST, "riga letta in formato non valido");

        }

        res.cnt++;

    }

    CHECK_END_FGETS(fd, res, "Request file processed and messages sent successfully\n");
    CHECK_FCLOSE(fd,res,"errore nella chiusura del file");
    
    return res;
}

