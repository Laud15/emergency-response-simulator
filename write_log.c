#include "include/write_log.h"

const char* event_to_string(log_event event){
    switch (event){
    case 0:
        return "PARSE FILE ENV";
    case 1:
        return "PARSE FILE RESCUERS";
    case 2:
        return "PARSE FILE EMERGENCY TYPE";
    case 3:
        return "MSQ";
    case 4:
        return "EMERGENCY STATUS";
    case 5:
        return "RESCUER STATUS";
    case 6:
        return "RESCUER ASSIGNMENT";
    case 7:
        return "TIMEOUT FOR DISTANCE";
    case 8:
        return "TIMEOUT FOR RESOURCES";
    case 9:
        return "INVALID INPUT";
    case 10:
        return "CLIENT REQUEST";
    case 11:
        return "SHUTDOWN REQUEST";
    default:
        return "NO_REG_EVENT";
    }
}

void write_log(long timestamp, int id, log_event event, char* msg){

    int fd, len;
    char buffer[BUFFERSIZE];
    char buffer_date[LEN_DATE];

    // The log file must already exist; each event is appended with one write.
    SCALL(fd,open(PATH_LOG, O_WRONLY | O_APPEND, 0620), "errore nell'apertura del file di log");
    
    strftime(buffer_date, LEN_DATE, "%Y-%m-%d %H:%M:%S", localtime(&timestamp));
    
    SCALL(len, snprintf(buffer, BUFFERSIZE,"[%s] [%d] [%s]  %s\n",buffer_date, id, event_to_string(event), msg), "errore nella snprintf");

    EQ_ERROR(write(fd,buffer,len), "errore nella scrittura nel file di log");
    EQ_ERROR(close(fd), "errore nella chiusura del file di log");

    return;
}