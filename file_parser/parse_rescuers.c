#include "../file_header/parse_rescuers.h"
#include "../file_header/write_log.h"

// Load [name] [count] [speed] [base_x;base_y] rows into responder pools.
parser_result_t parse_rescuers(rescuer_pool_t** rescuer_pools){
    
    FILE* fd;
    parser_result_t res;
    res.cnt=0;
    res.error=0;
    char buffer[BUFFERSIZE];
    int type_capacity= START_LST_DIM;
    int num_tmp, speed_tmp, x_tmp, y_tmp;
    int sscanf_res;
    
    SNCALL(fd, fopen(PATH_RESCUERS,"r"), "Errore nell'apertura del file rescuers.conf");
    write_log(time(NULL), operation_id, FILE_PARSING_RESCUERS, "apertura file rescuers.conf");

    NCALL_PARSER((*rescuer_pools), malloc(sizeof(rescuer_pool_t)*START_LST_DIM), "Errore nell'allocazione di memoria iniziale", res, fd);
    
    while(fgets(buffer, BUFFERSIZE, fd)!=NULL){
        
        if((res.cnt)==type_capacity){
            // Preserve the current allocation if realloc fails.
            rescuer_pool_t* lst_tmp;
            NCALL_PARSER(lst_tmp, realloc(*rescuer_pools, sizeof(rescuer_pool_t)*(type_capacity*2)),"Errore nella reallocazione della lista", res, fd);
            *rescuer_pools=lst_tmp;
            type_capacity*=2;
        }
 
        rescuer_type_t *rescuer;
        
        NCALL_PARSER(rescuer, malloc(sizeof(rescuer_type_t)),"errore per l'allocazione per lo spazio nuovo rescuer", res, fd);
        NCALL_PARSER(rescuer->rescuer_type_name,malloc(RESCUER_NAME_LENGTH*sizeof(char)),"errore nell'allocazione di memoria per il nome del rescuer", res, fd);
        
        sscanf_res = sscanf(buffer, "[%63[^]]] [%d] [%d] [%d;%d]",rescuer->rescuer_type_name, &num_tmp, &speed_tmp, &x_tmp, &y_tmp);
        
        CHECK_SSCANF(sscanf_res, 5, buffer, res, fd);

        write_log(time(NULL), operation_id, FILE_PARSING_RESCUERS, buffer);
        rescuer->speed=speed_tmp;
        rescuer->x=x_tmp;
        rescuer->y=y_tmp;
        
        (*rescuer_pools)[res.cnt].idle_count= num_tmp;
        
        NCALL_PARSER(((*rescuer_pools)[res.cnt].instances), malloc(sizeof(rescuer_digital_twin_t)*num_tmp), "errore nell'allocazione dello spazio per i digital twin", res, fd);
        
        // Instances in a pool share a type descriptor and start at its base.
        for(size_t i=0; i<num_tmp;i++){
            (*rescuer_pools)[res.cnt].instances[i].id=i+1;
            (*rescuer_pools)[res.cnt].instances[i].x=x_tmp;
            (*rescuer_pools)[res.cnt].instances[i].y=y_tmp;
            (*rescuer_pools)[res.cnt].instances[i].rescuer=rescuer;
            (*rescuer_pools)[res.cnt].instances[i].status=IDLE;
        }
        res.cnt++;
    }
    
    CHECK_END_FGETS(fd,res,"Rescuer pools loaded successfully\n");
    CHECK_FCLOSE(fd,res,"errore nella chiusura del file");
    write_log(time(NULL), operation_id, FILE_PARSING_RESCUERS, "chiusura file rescuers.conf");

    return res;
}

