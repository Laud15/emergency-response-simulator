#include "../file_header/parse_emergency_types.h"
#include "../file_header/write_log.h"

res_parser parse_emergency_types(emergency_type_t** lst, coppie_numero_gemelli* lista_soccorritori, int len_lista_soccorritori){
    
    FILE* fd; //dichiaro il file descriptor
    res_parser res;//dichiaro e poi inizializzo il valore di ritorno
    res.cnt=0;
    res.error=0;
    char buffer[BUFFERSIZE];//dichiaro il buffer da cui leggero le linee del file
    char str_rescuer_request[BUFFERSIZE];//buffer che conterrà la stringa delle richieste di soccorritori
    int dim_lista=START_LST_DIM, dim_lista_req;//dichiaro la dimensione iniziale della lista e dichiaro la dimensione iniziale della lista di richieste
    int sscanf_res, sscanf_lista_rescue;//dichiaro le due variabili che terrano il valore delle sscanf
    short prio;//dichiaro la varibile che terrà la priorità
    int req_number;//dichiaro la variabile che terrà il numero di tipi di richista
    char nome[RESCUER_NAME_LENGTH];//dichiaro il buffer per il nome del rescuer richiesto
    int num,time_to_manage;//dichiaro le varibile che immagazzineranno il numero richisto e il tempo necessario
    
    SNCALL(fd, fopen(PATH_EMERGENCY_TYPES,"r"), "errore nell'apertura del file emergency_types.conf");//apro (e controllo se l'apertura va a buon fine) il file
    write_log(time(NULL), id_operazione, FILE_PARSING_EMERGENCY_T, "apertura file emergency_types.conf");

    NCALL_PARSER(*lst, malloc(sizeof(emergency_type_t)*dim_lista), "errore nell'allocazione della memoria", res, fd);//alloco lo spazio per la lista che conterrà le emergenze

    while(fgets(buffer, BUFFERSIZE, fd)!=NULL){//leggo linea per linea dal fd
        req_number=0; //inizializzo il numero di richieste di cui ogni emergenza necessita

        if(res.cnt==dim_lista){//in caso sia arrivato in fondo alla lista realloco il doppio dello spazio
            emergency_type_t* lst_tmp;//utilizzo un puntatore temporaneo per fare il realloc per evitare di perdere il puntatore alla memoria già allocata
            NCALL_PARSER(lst_tmp, realloc(*lst, (sizeof(emergency_type_t)*(dim_lista*2))), "Errore nella reallocazione della memoria", res, fd);
            *lst=lst_tmp;
            dim_lista*=2;
        }
       
        NCALL_PARSER((*lst)[res.cnt].emergency_desc, malloc(EMERGENCY_NAME_LENGTH*(sizeof(char))), "errore nell'allocazione di spazio per il tipo di emergenza", res, fd);//alloco lo spazio per il descrittore dell'emergenza
       
        //dichiaro il puntatore alla stringa (che poi andrà parsata) delle richieste
        sscanf_res = sscanf(buffer, "[%127[^]]] [%hd] %1023[^\n]", (*lst)[res.cnt].emergency_desc, &prio, str_rescuer_request);//suddivido la linea letta in:descrittore, priorità,stringa delle richieste
        CHECK_SSCANF(sscanf_res, 3, buffer, res, fd);//controllo che sscanf abbia letto il numero giusto di campi

        //Mini-parser della stringa di richieste
        NCALL_PARSER((*lst)[res.cnt].rescuers, malloc(sizeof(rescuer_request_t)*START_LST_DIM), "errore nell'allocazione della memoria per le richieste di soccorritori",res, fd);//alloco lo spazio per la lista di richieste di soccorritori
        dim_lista_req=START_LST_DIM;
        char* token = strtok(str_rescuer_request,";");//inizio a tokenizzare la stringa delle richieste per ogni ";" trovato

        while(token != NULL){
            
            sscanf_lista_rescue=sscanf(token,"%63[^:]:%d,%d", nome, &num, &time_to_manage);//estraggo dal token il tipo di soccorritore, il numero richiesto e il tempo di gestione
            CHECK_SSCANF(sscanf_lista_rescue, 3, token, res, fd);//controllo che sscanf abbia letto il numero giusto di campi

            if(dim_lista_req==req_number){ //gestisco la reallocazione di memoria
                    rescuer_request_t* resc_tmp;
                    NCALL_PARSER(resc_tmp, realloc((*lst)[res.cnt].rescuers,sizeof(rescuer_request_t)*(dim_lista_req*2)),"erorre nella reallocazione di memoria per la lista di richieste", res, fd);
                    (*lst)[res.cnt].rescuers=resc_tmp;
                    dim_lista_req*=2;
            }
            
            for(size_t i=0;i<len_lista_soccorritori;i++){//scorro la lista di soccorritori
                if((strcmp(lista_soccorritori[i].lst_gemelli[0].rescuer->rescuer_type_name, nome))==0){//controllo se il nome del tipo di soccorritori e quello parsato combaciano (do per assodato che il file di emergenze contenga richieste di soccorritori sempre esistenti)
                    (*lst)[res.cnt].rescuers[req_number].type=lista_soccorritori[i].lst_gemelli[0].rescuer;//se lo fanno assegno il puntatore al campo type dell'elemento 
                    (*lst)[res.cnt].rescuers[req_number].time_to_manage=time_to_manage;//assegno il tempo per gestire l'emergenza
                    (*lst)[res.cnt].rescuers[req_number].required_count=num;//assegno il numero di soccorritori richiesti
                    break;
                }
            }

            req_number++;//incremento il numero di richieste di tipi di soccorritori dell'emergenza
            token = strtok(NULL,";");//avanzo nella tokenizzazione
            
        }
        //Fine mini-parser
        
        (*lst)[res.cnt].rescuers_req_number=req_number;//assegno il numero di richieste di tipi diversi di soccorritori
        (*lst)[res.cnt].priority=prio;//assegno al campo priority la priorità letta
        write_log(time(NULL), id_operazione, FILE_PARSING_EMERGENCY_T, buffer);
        res.cnt++;
    }

    CHECK_END_FGETS(fd,res,"parse emergency_types completato con successo\n");
    CHECK_FCLOSE(fd, res,"errore nella chisura del file");
    write_log(time(NULL), id_operazione, FILE_PARSING_EMERGENCY_T, "chiusura file emergency_types.conf");
    
    return res;
}

