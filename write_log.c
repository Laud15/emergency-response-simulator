#include "file_header/write_log.h"

const char* event_to_string(log_event event){
    switch (event){
    case 0:
        return "PARSE FILE ENV";
    case 1:
        return "PARSE FILE RESCUERS";
    case 2:
        return "PARSE FILE EMERGENCY TYPE";
    case 3:
        return "SHARED MEMORY";
    case 4:
        return "MSQ";
    case 5:
        return "EMERGENCY STATUS";
    case 6:
        return "RESCUER STATUS";
    case 7:
        return "RESCUER ASSIGNMENT";
    case 8:
        return "TIMEOUT FOR DISTANCE";
    case 9:
        return "TIMEOUT FOR RESOURCES";
    case 10:
        return "INVALID INPUT";
    case 11:
        return "CLIENT REQUEST";
    case 12:
        return "SHUTDOWN REQUEST";
    default:
        return "NO_REG_EVENT";
    }
}

void write_log(long timestamp, int id, log_event event, char* msg){

    int fd, len;
    char buffer[BUFFERSIZE];
    char buffer_date[LEN_DATE];

    SCALL(fd,open(PATH_LOG, O_WRONLY | O_APPEND, 0620), "errore nell'apertura del file di log");
    
    strftime(buffer_date, LEN_DATE, "%Y-%m-%d %H:%M:%S", localtime(&timestamp));//converto il tempo in un formato più leggibile
    
    SCALL(len, snprintf(buffer, BUFFERSIZE,"[%s] [%d] [%s]  %s\n",buffer_date, id, event_to_string(event), msg), "errore nella snprintf");//"formatto" la stringa che andrò poi a scrivere nel file di log


    EQ_ERROR(write(fd,buffer,len), "errore nella scrittura nel file di log");//scrivo la riga formattata
    EQ_ERROR(close(fd), "errore nella chiusura del file di log");//chiudo il file

    return;
}