#include "../file_header/parse_rescuers.h"
#include "../file_header/write_log.h"

//Parse rescuers prende in input una lista da popolare e ritorna un intero che rappresenta la lunghezza della lista
res_parser parse_rescuers(coppie_numero_gemelli** lista_coppie){ //passo come argomento un puntatore al primo puntatore della lista che voglio popolare
    
    FILE* fd; //dichiaro il descrittore del file
    res_parser res; //il valore di ritorno è una coppia in cui il primo elemento indica il numero di righe lette dal parse e il secondo indica se ci sono stati errori
    res.cnt=0;
    res.error=0;
    char buffer[BUFFERSIZE]; //utilizzo un buffer dal 1024 caratteri
    int dim_lista= START_LST_DIM; //dimensione della lista in partenza
    int num_tmp, speed_tmp, x_tmp, y_tmp; //dichiaro le variabili che utilizzerò per prendere i valori letti
    int sscanf_res; //variabile per leggere il risultato di sscanf
    
    SNCALL(fd, fopen(PATH_RESCUERS,"r"), "Errore nell'apertura del file rescuers.conf"); //apro il file rescuer.conf
    write_log(time(NULL), id_operazione, FILE_PARSING_RESCUERS, "apertura file rescuers.conf");

    NCALL_PARSER((*lista_coppie), malloc(sizeof(coppie_numero_gemelli)*START_LST_DIM), "Errore nell'allocazione di memoria iniziale", res, fd);
    
    while(fgets(buffer, BUFFERSIZE, fd)!=NULL){ //leggo la linea e la metto in un buffer di 1024 byte
        
        if((res.cnt)==dim_lista){ //se arrivo alla fine della lista realloco il doppio della memoria
            coppie_numero_gemelli* lst_tmp; //dichiaro un puntatore così,in caso la realloc ritornasse null , non perderei il puntatore alla lista originale e potrei fare la free
            NCALL_PARSER(lst_tmp, realloc(*lista_coppie, sizeof(coppie_numero_gemelli)*(dim_lista*2)),"Errore nella reallocazione della lista", res, fd);
            *lista_coppie=lst_tmp;
            dim_lista*=2;
        }
 
        rescuer_type_t *rescuer; //dichiaro lo spazio per il rescuer appena letto e gli assegno i valori
        
        NCALL_PARSER(rescuer, malloc(sizeof(rescuer_type_t)),"errore per l'allocazione per lo spazio nuovo rescuer", res, fd);//alloco spazio per la struttura del rescue
        NCALL_PARSER(rescuer->rescuer_type_name,malloc(RESCUER_NAME_LENGTH*sizeof(char)),"errore nell'allocazione di memoria per il nome del rescuer", res, fd);//alloco spazio per il nome del rescue
        
        sscanf_res = sscanf(buffer, "[%63[^]]] [%d] [%d] [%d;%d]",rescuer->rescuer_type_name, &num_tmp, &speed_tmp, &x_tmp, &y_tmp);
        
        CHECK_SSCANF(sscanf_res, 5, buffer, res, fd);//controllo che sscanf abbia letto il giusto numero di campi

        write_log(time(NULL), id_operazione, FILE_PARSING_RESCUERS, buffer);
        //assegno le variabili lette alla struttura rescuers appena creata
        rescuer->speed=speed_tmp;
        rescuer->x=x_tmp;
        rescuer->y=y_tmp;
        
        (*lista_coppie)[res.cnt].n= num_tmp; //assegno al campo n dell'elemento corrente della lista il numero di gemelli del rescuers e poi alloco lo spazio 
        
        NCALL_PARSER(((*lista_coppie)[res.cnt].lst_gemelli), malloc(sizeof(rescuer_digital_twin_t)*num_tmp), "errore nell'allocazione dello spazio per i digital twin", res, fd);
        
        for(size_t i=0; i<num_tmp;i++){ //assegno i valori a tutti i gemelli
            (*lista_coppie)[res.cnt].lst_gemelli[i].id=i+1;
            (*lista_coppie)[res.cnt].lst_gemelli[i].x=x_tmp;
            (*lista_coppie)[res.cnt].lst_gemelli[i].y=y_tmp;
            (*lista_coppie)[res.cnt].lst_gemelli[i].rescuer=rescuer;
            (*lista_coppie)[res.cnt].lst_gemelli[i].status=IDLE;
        }
        res.cnt++; //incremento il contatore che mi tiene traccia dei vari tipi di rescuer
    }
    
    CHECK_END_FGETS(fd,res,"parse rescuers completato con successo\n");
    CHECK_FCLOSE(fd,res,"errore nella chiusura del file");
    write_log(time(NULL), id_operazione, FILE_PARSING_RESCUERS, "chiusura file rescuers.conf");

    return res;
}
    


