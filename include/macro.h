#ifndef MACRO_H
#define MACRO_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <mqueue.h>        
#include <threads.h>
#include <stdatomic.h>
#include <time.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

#define START_LST_DIM 5
#define BUFFERSIZE 1024
#define LEN_DATE 128
#define SCALL_ERROR -1
#define NUM_THREAD_SERVER 10
#define PRI_0 0
#define PRI_1 1
#define PRI_2 2
#define STRCMP_TRUE 0

extern int operation_id;

#define PATH_LOG "log_file.log"
#define PATH_RESCUERS "config/rescuers.conf"
#define PATH_EMERGENCY_TYPES "config/emergency_types.conf"
#define PATH_ENV "config/env.conf"

// Error-checking helpers adapted from course material.

#define SCALL(r, c, e) do { if((r = (c)) == SCALL_ERROR) { perror(e); exit(EXIT_FAILURE); } } while(0) 
#define SNCALL(r, c, e) do { if((r = (c)) == NULL) { perror(e); exit(EXIT_FAILURE); } } while(0) 

// Project-specific parser, cleanup and timing helpers.

// Parser checks report failure through parser_result_t and may return early.

#define CHECK_FCLOSE(fd,res,e) do{ if((fclose(fd)) == EOF) { perror(e); res.error=SCALL_ERROR; return res; } } while(0)
#define NCALL_PARSER(r,c,e,res,fd) do { if((r = (c)) == NULL) { perror(e); ((res.error)=SCALL_ERROR); CHECK_FCLOSE(fd, res,"errore nella chiusura del file"); return res; } } while(0)

// Free responder arrays and shared type data for pools with a positive idle count.
#define FREE_ALL_RESCUE(list, len_list) do{\
    if((*list) != NULL){\
        for(size_t i=0;i<len_list;i++){\
            if(((*list)[i].instances) != NULL && (*list)[i].idle_count > 0){\
                if(((*list)[i].instances[0].rescuer) !=NULL){\
                    if((((*list)[i]).instances[0].rescuer->rescuer_type_name) != NULL){\
                        free(((*list)[i].instances[0].rescuer->rescuer_type_name));\
                        (*list)[i].instances[0].rescuer->rescuer_type_name=NULL;\
                    } \
                    free((*list)[i].instances[0].rescuer);\
                    (*list)[i].instances[0].rescuer=NULL;\
                }\
                free((*list)[i].instances);\
                (*list)[i].instances=NULL;\
            }\
        }\
        free(*list);\
        *list=NULL;\
    }\
 }while(0)

// Free emergency names and requirement arrays; responder types are borrowed.
 #define FREE_ALL_EMERGENCY(list, n) do{\
    if(*list != NULL){\
        for(size_t i=0;i<n;i++){\
            if((*list)[i].emergency_desc != NULL){\
                free((*list)[i].emergency_desc);\
                (*list)[i].emergency_desc=NULL;\
            }\
            if((*list)[i].rescuers != NULL){\
                free((*list)[i].rescuers);\
                (*list)[i].rescuers=NULL;\
            }\
        }\
        free(*list);\
        *list=NULL;\
    }\
}while(0)

#define CHECK_END_FGETS(fd,res,e) do{\
    if(feof(fd)){\
        printf(e);\
    }else if(ferror(fd)){\
        perror("lettura interrotta per motivo sconosciuto\n");\
        CHECK_FCLOSE(fd,res, "errore nella chiusura del file");\
        res.error=SCALL_ERROR;\
        return res;\
    }else{\
        fprintf(stderr, "Read interrupted for an unknown reason\n");\
        CHECK_FCLOSE(fd, res, "errore nella chiusura del file");\
        res.error=SCALL_ERROR;\
        return res;\
    }\
}while(0)\

#define CHECK_SSCANF(sscanf, n, buffer, res, fd) do{\
    if(sscanf != n){\
        fprintf(stderr,  "Failed to parse line: %s", buffer);\
        CHECK_FCLOSE(fd, res,"errore nella chiusura del file");\
        res.error=SCALL_ERROR;\
        return res;\
    }\
}while(0)

// Grid calculations and general-purpose fatal error checks.

#define ABS(x) ((x<0) ? (x*(-1)) : x)
// Manhattan distance; travel time uses integer division by speed.
#define MANHATTAN_DISTANCE(x1,y1,x2,y2) (ABS((x1-x2))+ABS((y1-y2)))
#define TRAVEL_TIME(speed, x1, y1, x2, y2) ((MANHATTAN_DISTANCE(x1, y1, x2, y2))/speed)
#define POS_AND_MIN(x, n) (((x) > 0 && (x) <= (n)) ? 1 : 0)

#define THRD_CHECK(t, e)do{if(t==thrd_error){perror(e);exit(EXIT_FAILURE);}}while(0)
#define EQ_ERROR(n, e) do{if(n==SCALL_ERROR){perror(e);exit(EXIT_FAILURE);} }while(0)
#define EQN_ERROR(n, e) do{if(n==NULL){perror(e);exit(EXIT_FAILURE);} }while(0)

#endif
