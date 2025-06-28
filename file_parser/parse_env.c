#include "../file_header/parse_env.h"
#include "../file_header/write_log.h"

res_parser parse_env(env* ambiente){
    res_parser res;
    res.cnt=0;
    res.error=0;
    FILE* fd;
    char buffer[BUFFERSIZE];
    char tmp[QUEUE_NAME_LENGTH];//stringa temporanea che utilizzo per l'aggiunta di / davanti al nome della mq
    

    SNCALL(fd, fopen(PATH_ENV, "r"), "errore nell'apertura del file env.conf");
    write_log(time(NULL), id_operazione, FILE_PARSING_ENV, "apertura file env.conf andata a buon fine");

    //parso il nome della msg_queue
    fgets(buffer, BUFFERSIZE, fd);
    CHECK_SSCANF((sscanf(buffer,"queue=%62[^\n]", tmp)), 1, buffer, res, fd);
    ambiente->nome_msg_queue[0]='/';
    strcpy(&(ambiente->nome_msg_queue[1]), tmp);
    write_log(time(NULL), id_operazione, FILE_PARSING_ENV, buffer);
    res.cnt++;

    //prendo l'altezza
    fgets(buffer, BUFFERSIZE, fd);
    CHECK_SSCANF((sscanf(buffer,"heigth=%d\n", &(ambiente->height))), 1, buffer, res, fd);
    write_log(time(NULL), id_operazione, FILE_PARSING_ENV, buffer);
    res.cnt++;

    //prendo la larghezza
    fgets(buffer, BUFFERSIZE, fd);
    sscanf(buffer,"width=%d\n", &(ambiente->width));
    CHECK_SSCANF((sscanf(buffer,"width=%d\n", &(ambiente->width))), 1, buffer, res, fd);
    write_log(time(NULL), id_operazione, FILE_PARSING_ENV, buffer);
    res.cnt++;

    CHECK_FCLOSE(fd,res,"errore nella chiusura del file\n");
    write_log(time(NULL), id_operazione, FILE_PARSING_ENV, "chiusura file env");
    return res;
}
