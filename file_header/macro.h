#ifndef MACRO_H
#define MACRO_H

#define _POSIX_C_SOURCE 200809L

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

extern atomic_int go;
extern int id_operazione;

#define SHM_NAME "/shared_memory"
#define PATH_LOG "log_file.log"
#define PATH_RESCUERS "file_conf/rescuers.conf"
#define PATH_EMERGENCY_TYPES "file_conf/emergency_types.conf"
#define PATH_ENV "file_conf/env.conf"

//-----------MACRO PRESE DALLE LEZIONI--------------------------------------------------------//

#define SCALL(r, c, e) do { if((r = (c)) == SCALL_ERROR) { perror(e); exit(EXIT_FAILURE); } } while(0) 
#define SNCALL(r, c, e) do { if((r = (c)) == NULL) { perror(e); exit(EXIT_FAILURE); } } while(0) 

//--------------------MACRO FATTE DA ME------------------------------------------------------//


//FREE_ALL_RESCUE:prende come argoment serve a liberare prima il nome del rescuer, poi la locazione di memoria adibita per la struttra del rescuer, poi libera la lista dei gemelli e infine libera la lista contenente le coppie numero_gemello, setta anche tutto a null
//FREE_ALL_EMERGENCY: libera e setta a null una lista passata come argomento di tipo lista_emergency** e la lista libera la memoria. Si presuppone che si usi prima la free resucers e per questo non si vanno a fare free ai rescuer puntati
//NCALL_PARSER: la si usa per qualsiasis chiamata (di sistema e non) che ritorni null in caso di errore all'interno del parser (solo dopo aver aperto il file) setta il campo error del tipo di ritorno del parse a -1
//ABS:calcola il valore assoluto di un numero 
//DISTANZA:calcola la distanza, mettendo in valore assoluto i due operando della somma
//TRAVEL_TIME:effettua distanza/velocità
//POS_AND_MIN:controlla che una valore sia positivo e minore di un altro valore
//EQ_ERROR:simile a scall con la differenza che non necessita una variabile dove inserire il risultato
//EQN_ERROR:come EQ_ERROR ma con null
//CHECK_FCLOSE:controlla la chisura del file è andata a buon fine, prende come argomento il tipo di ritorno dei parse e setta l'errore poi ritorna
//CHECK_END_FGETS:controlla che si sia arrivati effettivamente a fine file con la fgets
//CHECK_SSCANF:controlla che i caratteri letti dalla sscanf corrispando ad un intero, come per le altre funzioni dei parse prende anche il res del parser e vi setta l'errore prima di ritornare
//CHECK_CREATE:Controlla che la creazione di un thread vada a buon fine
//CHECK_JOIN:Controlla che la join di un thread vada a buon fine



//MACRO PARSER-EXCLUSIVE (utilizzabili solo in presenza di una struttura res_parser e da affiancare poi ad una reale terminazione)

#define CHECK_FCLOSE(fd,res,e) do{ if((fclose(fd)) == EOF) { perror(e); res.error=SCALL_ERROR; return res; } } while(0)
#define NCALL_PARSER(r,c,e,res,fd) do { if((r = (c)) == NULL) { perror(e); ((res.error)=SCALL_ERROR); CHECK_FCLOSE(fd, res,"errore nella chiusura del file"); return res; } } while(0)

#define FREE_ALL_RESCUE(list, len_list) do{\
    if((*list) != NULL){\
        for(size_t i=0;i<len_list;i++){\
            if(((*list)[i].lst_gemelli) != NULL && (*list)[i].n > 0){\
                if(((*list)[i].lst_gemelli[0].rescuer) !=NULL){\
                    if((((*list)[i]).lst_gemelli[0].rescuer->rescuer_type_name) != NULL){\
                        free(((*list)[i].lst_gemelli[0].rescuer->rescuer_type_name));\
                        (*list)[i].lst_gemelli[0].rescuer->rescuer_type_name=NULL;\
                    } \
                    free((*list)[i].lst_gemelli[0].rescuer);\
                    (*list)[i].lst_gemelli[0].rescuer=NULL;\
                }\
                free((*list)[i].lst_gemelli);\
                (*list)[i].lst_gemelli=NULL;\
            }\
        }\
        free(*list);\
        *list=NULL;\
    }\
 }while(0)

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
        fprintf(stderr, "lettura interrotta per motivo sconosciuto\n");\
        CHECK_FCLOSE(fd, res, "errore nella chiusura del file");\
        res.error=SCALL_ERROR;\
        return res;\
    }\
}while(0)\

#define CHECK_SSCANF(sscanf, n, buffer, res, fd) do{\
    if(sscanf != n){\
        fprintf(stderr,  "Errore nel parsing della riga: %s", buffer);\
        CHECK_FCLOSE(fd, res,"errore nella chiusura del file");\
        res.error=SCALL_ERROR;\
        return res;\
    }\
}while(0)


//____________MACRO NON PARSER-EXCLUSIVE___________________________________________________

#define ABS(x) ((x<0) ? (x*(-1)) : x)
#define DISTANZA(x1,y1,x2,y2) (ABS((x1-x2))+ABS((y1-y2)))
#define TRAVEL_TIME(vel, x1, y1, x2, y2) ((DISTANZA(x1, y1, x2, y2))/vel)
#define POS_AND_MIN(x, n) (((x) > 0 && (x) <= (n)) ? 1 : 0)

#define THRD_CHECK(t, e)do{if(t==thrd_error){perror(e);exit(EXIT_FAILURE);}}while(0)
#define MMAP(r, c, e) do { if((r = (c)) == MAP_FAILED) { perror(e); exit(EXIT_FAILURE); } } while(0)
#define EQ_ERROR(n, e) do{if(n==SCALL_ERROR){perror(e);exit(EXIT_FAILURE);} }while(0)
#define EQN_ERROR(n, e) do{if(n==NULL){perror(e);exit(EXIT_FAILURE);} }while(0)



#endif