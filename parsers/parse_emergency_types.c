#include "../include/parse_emergency_types.h"
#include "../include/write_log.h"

// Load [name] [priority] type:count,service_seconds;... rows.
parser_result_t parse_emergency_types(emergency_type_t** lst, rescuer_pool_t* rescuer_pools, int rescuer_pool_count){
    
    FILE* fd;
    parser_result_t res;
    res.cnt=0;
    res.error=0;
    char buffer[BUFFERSIZE];
    char str_rescuer_request[BUFFERSIZE];
    int type_capacity=START_LST_DIM, requirement_capacity;
    int sscanf_res, requirement_fields_read;
    short prio;
    int req_number;
    char rescuer_name[RESCUER_NAME_LENGTH];
    int num,time_to_manage;
    
    SNCALL(fd, fopen(PATH_EMERGENCY_TYPES,"r"), "errore nell'apertura del file emergency_types.conf");
    write_log(time(NULL), operation_id, FILE_PARSING_EMERGENCY_T, "apertura file emergency_types.conf");

    NCALL_PARSER(*lst, malloc(sizeof(emergency_type_t)*type_capacity), "errore nell'allocazione della memoria", res, fd);

    while(fgets(buffer, BUFFERSIZE, fd)!=NULL){
        req_number=0;

        if(res.cnt==type_capacity){
            emergency_type_t* lst_tmp;
            NCALL_PARSER(lst_tmp, realloc(*lst, (sizeof(emergency_type_t)*(type_capacity*2))), "Errore nella reallocazione della memoria", res, fd);
            *lst=lst_tmp;
            type_capacity*=2;
        }
       
        NCALL_PARSER((*lst)[res.cnt].emergency_desc, malloc(EMERGENCY_NAME_LENGTH*(sizeof(char))), "errore nell'allocazione di spazio per il tipo di emergenza", res, fd);
       
        sscanf_res = sscanf(buffer, "[%127[^]]] [%hd] %1023[^\n]", (*lst)[res.cnt].emergency_desc, &prio, str_rescuer_request);
        CHECK_SSCANF(sscanf_res, 3, buffer, res, fd);

        // Parse the semicolon-separated resource requirements.
        NCALL_PARSER((*lst)[res.cnt].rescuers, malloc(sizeof(rescuer_request_t)*START_LST_DIM), "errore nell'allocazione della memoria per le richieste di soccorritori",res, fd);
        requirement_capacity=START_LST_DIM;
        char* token = strtok(str_rescuer_request,";");

        while(token != NULL){
            
            requirement_fields_read=sscanf(token,"%63[^:]:%d,%d", rescuer_name, &num, &time_to_manage);
            CHECK_SSCANF(requirement_fields_read, 3, token, res, fd);

            if(requirement_capacity==req_number){
                    rescuer_request_t* resc_tmp;
                    NCALL_PARSER(resc_tmp, realloc((*lst)[res.cnt].rescuers,sizeof(rescuer_request_t)*(requirement_capacity*2)),"erorre nella reallocazione di memoria per la lista di richieste", res, fd);
                    (*lst)[res.cnt].rescuers=resc_tmp;
                    requirement_capacity*=2;
            }
            
            // Configuration assumes every requested responder type exists.
            for(size_t i=0;i<rescuer_pool_count;i++){
                if((strcmp(rescuer_pools[i].instances[0].rescuer->rescuer_type_name, rescuer_name))==STRCMP_TRUE){
                    (*lst)[res.cnt].rescuers[req_number].type=rescuer_pools[i].instances[0].rescuer;
                    (*lst)[res.cnt].rescuers[req_number].time_to_manage=time_to_manage;
                    (*lst)[res.cnt].rescuers[req_number].required_count=num;
                    break;
                }
            }

            req_number++;
            token = strtok(NULL,";");
            
        }
        
        (*lst)[res.cnt].rescuers_req_number=req_number;
        (*lst)[res.cnt].priority=prio;
        write_log(time(NULL), operation_id, FILE_PARSING_EMERGENCY_T, buffer);
        res.cnt++;
    }

    CHECK_END_FGETS(fd,res,"Emergency types loaded successfully\n");
    CHECK_FCLOSE(fd, res,"errore nella chisura del file");
    write_log(time(NULL), operation_id, FILE_PARSING_EMERGENCY_T, "chiusura file emergency_types.conf");
    
    return res;
}

