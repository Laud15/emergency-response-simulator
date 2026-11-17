#include "../file_header/parse_env.h"
#include "../file_header/write_log.h"

// Read queue, heigth and width in that order from the environment file.
parser_result_t parse_env(environment_t* environment){
    parser_result_t res;
    res.cnt=0;
    res.error=0;
    FILE* fd;
    char buffer[BUFFERSIZE];
    char tmp[QUEUE_NAME_LENGTH];

    SNCALL(fd, fopen(PATH_ENV, "r"), "errore nell'apertura del file env.conf");
    write_log(time(NULL), operation_id, FILE_PARSING_ENV, "apertura file env.conf andata a buon fine");

    // Prefix the configured queue name with the slash required by POSIX.
    fgets(buffer, BUFFERSIZE, fd);
    CHECK_SSCANF((sscanf(buffer,"queue=%62[^\n]", tmp)), 1, buffer, res, fd);
    environment->queue_name[0]='/';
    strcpy(&(environment->queue_name[1]), tmp);
    write_log(time(NULL), operation_id, FILE_PARSING_ENV, buffer);
    res.cnt++;

    // Keep the configuration key's existing "heigth" spelling.
    fgets(buffer, BUFFERSIZE, fd);
    CHECK_SSCANF((sscanf(buffer,"heigth=%d\n", &(environment->height))), 1, buffer, res, fd);
    write_log(time(NULL), operation_id, FILE_PARSING_ENV, buffer);
    res.cnt++;

    fgets(buffer, BUFFERSIZE, fd);
    sscanf(buffer,"width=%d\n", &(environment->width));
    CHECK_SSCANF((sscanf(buffer,"width=%d\n", &(environment->width))), 1, buffer, res, fd);
    write_log(time(NULL), operation_id, FILE_PARSING_ENV, buffer);
    res.cnt++;

    CHECK_FCLOSE(fd,res,"errore nella chiusura del file\n");
    write_log(time(NULL), operation_id, FILE_PARSING_ENV, "chiusura file env");
    return res;
}
