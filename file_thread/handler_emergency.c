#include "../file_header/handler_emergency.h"
#include "../file_header/barrier.h"
#include "../file_header/write_log.h"

#define TIME_PRI1 30
#define TIME_PRI2 10
#define TIME_OUT 0
#define IN_TIME 1
#define NOT_ALL_RESCUERS 0
#define ALL_RESCUERS 1

int pri2_cnt=0;//contatore che tiene traccia del numero di emergenze con priorità 2 in pausa
int pri1_cnt=0;//contatore che tiene traccia del numero di emergenze con priorità 1 in pausa

int conta_rescuer(emergency_type_t lista){//funzione che calcola il numero totale di soccorritori necessari per gestire l'emergenza
    int cnt=0;

    for(size_t i=0; i<lista.rescuers_req_number;i++){
        printf("richiesti %d di %s\n", lista.rescuers[i].required_count, lista.rescuers[i].type->rescuer_type_name);
        cnt+=lista.rescuers[i].required_count;
    }

    return cnt;
}

int tempo_sufficiente(emergency_type_t emergenza, int x_dest, int y_dest, struct timespec deadline, struct timespec start){//funzion che calcola e controlla se il tempo necessario per avere tutti i soccorritori sul luogo dell'emergenza è minore uguale a quello settato dalla priorità, ritorna 1 se c'è abbastanza tempo 0 altrimenti

    printf("\nsto calcolando il tempo sufficiente di %s <x:%d, y:%d>\n", emergenza.emergency_desc, x_dest, y_dest);
    int max=0;
    int travel_time;
    struct timespec now;
    timespec_get(&now, TIME_UTC);//prendo il tempo di quando è stata chiamata tempo_sufficiente
    int sec = (int)(now.tv_sec-start.tv_sec);//calcolo quanto tempo è passato da quando ho chiamato iniziato la risoluzione 

    printf("\ntempo trascorso dall'inizio di %s <x:%d, y:%d>:%d\n",emergenza.emergency_desc, x_dest, y_dest, sec);

    if(emergenza.priority>0){//se la priorità è 0 non ho limiti di tempo
        if(sec>=(deadline.tv_sec-start.tv_sec)){//se è passato più tempo del timer mi fermo e ritorno 0
            return TIME_OUT;
        }

        for(size_t i=0; i<emergenza.rescuers_req_number;i++){//mi scorro tutti i tipi di soccorritori richiesti dall'emergenza

            travel_time=TRAVEL_TIME(emergenza.rescuers[i].type->speed, emergenza.rescuers[i].type->x, emergenza.rescuers[i].type->y, x_dest, y_dest);//calcolo tempo di di arrivo

            if(travel_time>max){//prendo il massimo
                max=travel_time;
                //printf("tempo massimo stimato per %s = %d\n", emergenza.emergency_desc, max);
            }
        }

        if(max>(deadline.tv_sec-now.tv_sec)){//se il tempo di arrivo è maggiore del tempo che mi rimane mi fermo
            printf("tempo massimo di arrivo a %s <x:%d, y:%d>:%d è maggiore del tempo rimanente\n",emergenza.emergency_desc, x_dest, y_dest, max);
            return TIME_OUT;
        }
        printf("il tempo di arrivo a %s  <x:%d,y:%d> è %d, ok!\n\n", emergenza.emergency_desc, x_dest, y_dest, max);//se invece il tempo è sufficiente posso continuare
        return IN_TIME;
    }

    printf("il tempo di arrivo a %s  <x:%d,y:%d> è ok\n", emergenza.emergency_desc, x_dest, y_dest);//questo è il caso in cui ho priorità 0
    return IN_TIME;
    
}



int rescuer_thread(void* arg){

    data_rescuer_thread* resc_data = (data_rescuer_thread*) arg;//faccio il cast a data_rescuer_thread di arg
    int travel_time=TRAVEL_TIME(resc_data->twin->rescuer->speed, resc_data->twin->rescuer->x,resc_data->twin->rescuer->y, resc_data->dest_x, resc_data->dest_y);
    int expected=0;//valore che mi aspetto quando non ci sono più soccorritori sul luogo dell'emergenza
    int desired=1;//valore che voglio avere quando non ci sono più soccorritori sul luogo dell'emergenza
    char buffer[BUFFERSIZE]; //buffer usato per la scrittura nel log

    resc_data->twin->status=EN_ROUTE_TO_SCENE;//setto il soccorritore nello stato EN_ROUTE
    snprintf(buffer, BUFFERSIZE, "rescuer id: %d %s messo in stato EN_ROUTE_TO_SCENE", resc_data->twin->id ,resc_data->twin->rescuer->rescuer_type_name);
    write_log(time(NULL), resc_data->id, RESCUER_ASSIGNMENT, buffer);

    sleep(travel_time);//aspetto per il tempo di arrivo

    resc_data->twin->status=ON_SCENE;//dopo aver atteso l'arrivo mi metto on-scene
    snprintf(buffer, BUFFERSIZE, "rescuer id: %d %s messo in stato ON_SCENE", resc_data->twin->id ,resc_data->twin->rescuer->rescuer_type_name);
    write_log(time(NULL), resc_data->id, RESCUER_STATUS, buffer);
    
    barrier_em_status(resc_data->stato_emergenza, resc_data->count, (int)(*resc_data->n_twin), resc_data->mtx_barrier, resc_data->cnd_barrier, resc_data->id);//aspetto che arrivino tutti, nel momento in cui arrivano l'ultimo setto lo stato a IN_PROGRESS

    sleep(resc_data->time_manage);//mi fermo per il tempo richiesto
    printf("sono %d %s e ho finito il mio lavoro\n", resc_data->twin->id,resc_data->twin->rescuer->rescuer_type_name);

    //DECREMENTO IL NUMERO DI SOCCORRITORI ANCORA SUL CAMPO
    atomic_fetch_sub(resc_data->n_twin, 1);//quando ho finite diminuisco il numero di gemelli
    if(atomic_compare_exchange_strong(resc_data->n_twin,&expected, desired)){//se non c'è più nessuno l'emergenza è stata risolta
        printf("\n--emergenza a <x:%d, y:%d> risolta--\n\n", resc_data->dest_x, resc_data->dest_y);
        (*(resc_data->stato_emergenza))=COMPLETED;
        write_log(time(NULL), resc_data->id, EMERGENCY_STATUS, "emergenza COMPLETED");
    }
    
    resc_data->twin->status=RETURNING_TO_BASE;//cambio il mio stato
    snprintf(buffer, BUFFERSIZE, "rescuer id: %d %s messo in RETURNING TO BASE", resc_data->twin->id ,resc_data->twin->rescuer->rescuer_type_name);
    write_log(time(NULL), resc_data->id, RESCUER_STATUS, buffer);

    sleep(travel_time);//faccio passare il tempo per tornare alla base
    resc_data->twin->status=IDLE;//mi rimetto in idle

    snprintf(buffer, BUFFERSIZE, "rescuer id: %d %s tornato alla base, messo in IDLE", resc_data->twin->id ,resc_data->twin->rescuer->rescuer_type_name);
    write_log(time(NULL), resc_data->id, RESCUER_STATUS, buffer);

    //PARTE RISVEGLIO DELLE EMERGENZE MESSE IN PAUSA
    THRD_CHECK(mtx_lock(&mtx_call_rescuers), "errore nella lock di mtx_call_rescuer");//uso lo stesso lock della call perchè sto accedendo (e modificando) la lista condivisa tra tutti i thread

    //effettuo l'incremento dei gemelli disponibili
    for(size_t i=0; i<len_lst_rescuers;i++){
        if(strcmp(resc_data->twin->rescuer->rescuer_type_name, lista_rescuers[i].lst_gemelli[0].rescuer->rescuer_type_name)==STRCMP_TRUE){
            lista_rescuers[i].dt_idle++;
            break;
        }
    }

    if(pri2_cnt>0){//se ci sono emergenze con priorità 2 risveglio quelle
        printf("sveglio quelli con priorità 2\n");
        THRD_CHECK(cnd_broadcast(&new_idle_pri2), "errore nella broadcast di new_idle");

    }else if(pri1_cnt>0){//se non ci sono emergenze di priorità 2 ma ci sono di priorità 1
        printf("sveglio quelli con priorità 1\n");
        THRD_CHECK(cnd_broadcast(&new_idle_pri1), "errore nella broadcast di new_idle");
        
    }else{//se ci sono solo priorità 0
        printf("sveglio quelli con priorità 0\n");
        THRD_CHECK(cnd_broadcast(&new_idle_pri0), "errore nella broadcast di new_idle");
    }

    THRD_CHECK(mtx_unlock(&mtx_call_rescuers), "errore nella unlock di mtx_call_rescuer");
    //FINE PARTE RISVEGLIO DELLE EMERGENZE MESSE IN PAUSA

    free(resc_data);//libero la memoria allocata per il dato
    return thrd_success;
}


int solve(emergency_t* emergenza, int id){//funzio che presa un emergenza chiama n thread soccorritori, assegna loro i parametri e una funzione che simulerà il comportamento del soccorritore, li aspetta e ritorna
    
    thrd_t rescuers[emergenza->rescuer_count];//creo l'array che conterra i thread, di lunghezza pari al numero di gemelli richiesti
    atomic_int n_twin=(atomic_int)emergenza->rescuer_count;//creo una variabile atomica che conterrà il numero gemelli
    mtx_t mtx_barrier;//creo la mia copia della mutex per la barriera
    cnd_t cnd_barrier;//creo la mia copia della cond_var per la barrier
    int count_barrier=0;//dichiaro un contatore che utilizzerò poi nella barrier

    THRD_CHECK(mtx_init(&mtx_barrier, mtx_plain), "errore nella init di mtx_barrier");
    THRD_CHECK(cnd_init(&cnd_barrier), "errore nella init di cnd_barrier");
    
    //PARTE CREAZIONE DEI THRD-SOCCORRITORI
    for(size_t i=0; i<emergenza->rescuer_count;i++){//calcolo il tempo di arrivo

        data_rescuer_thread* data;//creo il dato che poi passerò come argomento al thread
        SNCALL(data, malloc(sizeof(data_rescuer_thread)), "errore nell'allocazione di memoria per i thread soccorritori");//alloco e controllo

        data->id=id;
        data->twin=emergenza->rescuers_dt[i];//assegno il gemello che dovra partire, risolvere e tornare
        data->dest_x=emergenza->x;//passo la x della destinazione 
        data->dest_y=emergenza->y;//passo la y della destinazione
        data->n_twin=&n_twin;//passo il numero di gemelli che saranno presenti
        data->stato_emergenza=&(emergenza->status);//passo il puntatore allo stato dell'emergenza
        data->cnd_barrier=&cnd_barrier;//passo la cnd della barrier
        data->mtx_barrier=&mtx_barrier;//passo il mtx della barrier
        data->count=&count_barrier;//passo il counter della barrier
       
        for(size_t j=0; j<emergenza->type.rescuers_req_number;j++){
            if(strcmp(data->twin->rescuer->rescuer_type_name, emergenza->type.rescuers[j].type->rescuer_type_name)==STRCMP_TRUE){//cotrollo per quanto tempo il soccorritore deve rimanere sul luogo dell'emergenza
                data->time_manage=emergenza->type.rescuers[j].time_to_manage;
            }
        }
        THRD_CHECK(thrd_create(&rescuers[i], rescuer_thread, data), "errore nella create dei rescuer thread");//creo il thread
    }
    //FINE PARTE CREAZIONE DEI THRD-SOCCORRITORI

    for(size_t i=0; i<emergenza->rescuer_count;i++){
        THRD_CHECK(thrd_join(rescuers[i], NULL), "errore nella join dei rescuer_thread");//attendo il ritorno del thread
    }
    
    mtx_destroy(&mtx_barrier);//distruggo il mutex
    cnd_destroy(&cnd_barrier);//distruggo la cond_var
    return 0;
}


int call_rescuers(emergency_t* emergenza){

    int current_position=0;//utilizzo una variabile di supporto per tener traccia di dove mi sono fermato nel popolamento della lista dei dt dell'emergenza
    //CONTROLLO CHE CI SIANO ABBASTANZA GEMELLI LIBERI PER SODDISFARE LA MIA RICHIESTA
    for(size_t i=0;i<emergenza->type.rescuers_req_number;i++){
        for(size_t j=0;j<len_lst_rescuers;j++){

            if(strcmp(emergenza->type.rescuers[i].type->rescuer_type_name, lista_rescuers[j].lst_gemelli[0].rescuer->rescuer_type_name)==STRCMP_TRUE){//controllo che il tipo corrisponda
                if(emergenza->type.rescuers[i].required_count>lista_rescuers[j].dt_idle){//se ci sono più dt di quel tipo richiesti di quelli disponibili ritorno il valore per il quale mi sospendo
                    return NOT_ALL_RESCUERS;
                }
            }
        }
    }
    //SE CI SONO METTO I GEMELLI LIBERI NELLA LISTA
    for(size_t i=0; i<emergenza->type.rescuers_req_number;i++){
        for(size_t j=0;j<len_lst_rescuers;j++){

            if(strcmp(emergenza->type.rescuers[i].type->rescuer_type_name, lista_rescuers[j].lst_gemelli[0].rescuer->rescuer_type_name)==STRCMP_TRUE){

                for(size_t k=0; k<emergenza->type.rescuers[i].required_count;k++){
                    emergenza->rescuers_dt[current_position]=&(lista_rescuers[j].lst_gemelli[(lista_rescuers[j].dt_idle-1)]);//inizio a prendere i gemelli dall fondo della lista (avrò quindi come prime scelte quelli con gli id maggiori)
                    printf("ho inserito <id:%d tipo:%s> nella posizione %d\n", lista_rescuers[j].lst_gemelli[(lista_rescuers[j].dt_idle-1)].id, lista_rescuers[j].lst_gemelli[(lista_rescuers[j].dt_idle-1)].rescuer->rescuer_type_name, current_position);
                    lista_rescuers[j].dt_idle--;
                    current_position++;
                }
            }
        }
    }
    return ALL_RESCUERS;
}


int try_solve(emergency_t* emergenza, int id){//funzione che preso in input un'emergenza, una lista contenete tutti i gemelli disponibili e la suea lunghezza prova ad asseggnare tutti i gemelli richiesti dall'emergenza, se si riesce chiama solve, non ci riesce si mette in attesa di qualche nuovo soccorritore in idle

    printf("sto provando a risolvere %s\n", emergenza->type.emergency_desc);

    struct timespec deadline;//conterrà i secondi entro cui dovrò avere tutti i soccorritori sul luogo
    struct timespec start;//indica il tempo attuale al momento della chiamata di try_solve
    int cnd_res;

    //---PARTE DI CONTROLLO DELLE TEMPISTICHE---
    start.tv_nsec=0;
    start.tv_sec=emergenza->time;
    timespec_get(&deadline, TIME_UTC);

    switch (emergenza->type.priority){

        case PRI_0://se sono a 0 non ho bisogno di tener traccia del tempo
            break;
        case PRI_1://se ho 1 assegno 30 secondi 
            deadline.tv_nsec=0;
            deadline.tv_sec+=TIME_PRI1;
            break;
        case PRI_2://se ho 2 assegno 10 secondi
            deadline.tv_nsec=0;
            deadline.tv_sec+=TIME_PRI2;
            break;
        default://non si verifica mai
        fprintf(stderr, "priorità non riconosciuta");
        return thrd_error;

    }
    
    if(emergenza->type.priority>0){//se la priorità prevede un timer calcolo se è possibile risolvere l'emergenza entro quel tempo
        if(tempo_sufficiente(emergenza->type,emergenza->x, emergenza->y, deadline, start)==TIME_OUT){

            printf("\n--emergenza %s TIMEOUT--\n\n", emergenza->type.emergency_desc);
            emergenza->status=TIMEOUT;
            write_log(time(NULL), id, TIMEOUT_FOR_DISTANCE, "TIMEOUT dell'emergenza");
            return thrd_success;

        }
    }
    //---FINE PARTE CONTROLLO TEMPISTICHE---

    //---INIZIO SEZIONE CRITICA---
    THRD_CHECK(mtx_lock(&mtx_call_rescuers), "errore nella lock di mtx_call_rescuers");//visto che devo accedere ad una struttura condivista tra tutti i thread effettuo una lock per evitare race condition
    
    while(call_rescuers(emergenza)==NOT_ALL_RESCUERS){

        emergenza->status=PAUSED;
        write_log(time(NULL), id, EMERGENCY_STATUS, "emergenza messa in PAUSE");

        if(emergenza->type.priority>PRI_0){//se ho necessita del time uso una cnd_timedwait

                if(emergenza->type.priority==PRI_2){
                    pri2_cnt++;
                    printf("\n---non ci sono tutti i soccorritori di cui ho bisogno, mi metto in pausa, siamo %d con prio 2 in attesa---\n", pri2_cnt);
                    cnd_res = cnd_timedwait(&new_idle_pri2, &mtx_call_rescuers, &deadline);
                }else{
                    pri1_cnt++;
                    printf("\n---non ci sono tutti i soccorritori di cui ho bisogno, mi metto in pausa, siamo %d con prio 1 in attesa---\n", pri1_cnt);
                    cnd_res = cnd_timedwait(&new_idle_pri1, &mtx_call_rescuers, &deadline);
                }

                THRD_CHECK(cnd_res, "errore nella cnd_timedwait");

                printf("\nsono %s <x:%d,y:%d> e ho saputo ci sono nuovi idle\n\n", emergenza->type.emergency_desc, emergenza->x, emergenza->y);
                emergenza->status=WAITING;
                write_log(time(NULL), id, EMERGENCY_STATUS, "emergenza messa in WAITING");

                if(emergenza->type.priority==PRI_2){
                    pri2_cnt--;
                    printf("\n---sono rimasti %d di priorità 2 in attesa---\n", pri2_cnt);
                }else{
                    pri1_cnt--;
                    printf("\n---sono rimasti %d di priorità 1 in attesa---\n", pri1_cnt);
                }

                if(cnd_res == thrd_timedout || (tempo_sufficiente(emergenza->type,emergenza->x, emergenza->y, deadline, start)==TIME_OUT)){//una volta risvegliato se mi sono risvegliato per via del timer e se ho abbastanza tempo per eseguire l'emergenza provo a riacquisire i gemelli

                    printf("\nsono %s <x:%d, y:%d> e ho aspettato per troppo tempo\n\n", emergenza->type.emergency_desc, emergenza->x, emergenza->y);
                    emergenza->status=TIMEOUT;//se non lo sono mi segno come timeout e ritorno
                    write_log(time(NULL), id, TIMEOUT_FOR_RESOURCE, "TIMEOUT dell'emergenza");
                    mtx_unlock(&mtx_call_rescuers);

                    return 0;
                }

            }else{//se invece ho priorità 0 non necessito del timer e usa una cnd normale
                THRD_CHECK(cnd_wait(&new_idle_pri0, &mtx_call_rescuers), "errore nella cnd_wait di new_idle");
                printf("\nsono %s <x:%d,y:%d> e ho saputo ci sono nuovi idle\n\n", emergenza->type.emergency_desc, emergenza->x, emergenza->y);
                emergenza->status=WAITING;
                write_log(time(NULL), id, EMERGENCY_STATUS, "emergenza messa in WAITING");
            }
    }

    printf("\n--ho trovato tutti i soccorritori di cui ho bisogno!--\n\n");
    for(size_t i=0;i<emergenza->rescuer_count;i++){
        emergenza->rescuers_dt[i]->status=EN_ROUTE_TO_SCENE;//visto che ho tutti i soccorritori setto il loro stato in modo da non essere disponibili per le altre emergenze
    }

    THRD_CHECK(mtx_unlock(&mtx_call_rescuers), "errore nella unlock di mtx_call_rescuers");
    //---FINE SEZIONE CRITICA---

    emergenza->status=ASSIGNED;//visto che ho tutti i soccorritori di cui ho bisogno
    write_log(time(NULL), id, EMERGENCY_STATUS, "Emergenza messa in ASSIGNED");

    solve(emergenza, id);

    return 1;
}


emergency_t elaborazione(emergency_request_t emergenza, int id){//funzione che "parsa" la struttura lettan nella mq e effetua i dovuti controlli

    emergency_t istanza_emergenza;//creo la struttura che conterrà il risultato dell'elaborazione
    int trovato=0;
    istanza_emergenza.rescuers_dt=NULL;

    if(POS_AND_MIN(emergenza.x, ambiente.width) && POS_AND_MIN(emergenza.y, ambiente.height)){//controllo che le coordinate siano corrette

            for(size_t i=0;i<len_lst_emergency_t;i++){

                if(strcmp(emergenza.emergency_name, lista_emergency_type[i].emergency_desc)==STRCMP_TRUE){//cerco l'emergenza richiesta (sono sicuro che esista perchè senno mi sarei fermato all'if precedente)

                    istanza_emergenza.type=lista_emergency_type[i];//assegno il tipo
                    istanza_emergenza.status=WAITING;//assegno lo stato in attesa
                    istanza_emergenza.x=emergenza.x;//assegno la coordinata x
                    istanza_emergenza.y=emergenza.y;//assegno la coordinata y
                    istanza_emergenza.time=emergenza.timestamp;//assegno il tempo in cui l'emergenza è accaduta
                    istanza_emergenza.rescuer_count=conta_rescuer(lista_emergency_type[i]);//assegno il numero dei rescuer di cui ho bisogno per terminare
                    SNCALL(istanza_emergenza.rescuers_dt, malloc(sizeof(rescuer_digital_twin_t*)*istanza_emergenza.rescuer_count), "errore nell'allocazione di memoria del campo rescuers_dt");//creo la lista che conterrà i puntatori ai gemelli

                    write_log(time(NULL), id, MESSAGE_QUEUE, "richiesta elaborata correttamente");
                    write_log(time(NULL), id, EMERGENCY_STATUS, "emergenza messa in stato di WAIT");
                    trovato=1;
                    break;
                }
            }

            if(!trovato){//Gestisco il caso in cui sia stata mandata un'emergenza non registrata nel file di configurazione
                write_log(time(NULL), id, MESSAGE_QUEUE, "richiesta emergenza non esistente");
                istanza_emergenza.status=CANCELED;
                write_log(time(NULL), id, EMERGENCY_STATUS, "richiesta CANCELED");
                return istanza_emergenza;
            }

        return istanza_emergenza;

    }else{//gestisco il caso di coordinate non valide
        istanza_emergenza.status=CANCELED;
        write_log(time(NULL), id, MESSAGE_QUEUE, "coordinate errate inserite");
        write_log(time(NULL), id, EMERGENCY_STATUS, "richiesta CANCELED");
        return istanza_emergenza;
    }
}


int handler_emergency(void* arg){

    char buffer[BUFFERSIZE];//buffer che si utilizza per la snprintf 
    int* id = (int*) arg;//faccio il cast del tipo
    emergency_request_t req;//creo la struttura che accoglierà il messaggio letto dalla mq
    
    if(mq_receive(mq, (char*)&req, sizeof(emergency_request_t), NULL) == SCALL_ERROR){
        write_log(time(NULL), *id, MESSAGE_QUEUE, "errore nella lettura della mq");
        perror("mq_receive");
        free(id);
        return thrd_error;
    }

    if(strcmp(req.emergency_name,"shutdown_server")==STRCMP_TRUE){//controllo se è stata fatta una richiesta di shutdown
        write_log(time(NULL), *id, SHUTDOWN, "richiesta di shutdown server");
        free(id);
        atomic_store(&go, 0);
        return thrd_success;
    }
    
    snprintf(buffer, BUFFERSIZE, "letto dalla mq emergenza:%s a <x:%d,y:%d>", req.emergency_name, req.x, req.y);
    write_log(time(NULL), *id, MESSAGE_QUEUE, buffer);

    emergency_t emergenza = elaborazione(req, *id);//assegno a emergenza il risultato di elaborazione
    
    if(emergenza.status!=CANCELED){//se elaborazione non ha trovato errori provo a risolvere l'emergenza
        printf("sono thrd %d e ho elaborato %s di priorità %hd a <x:%d, y:%d>\n", *id, emergenza.type.emergency_desc,emergenza.type.priority, emergenza.x, emergenza.y);
        try_solve(&emergenza, *id);
    }else{
        printf("sono thrd %d , emergenza non valida\n", *id);
    }

    free(emergenza.rescuers_dt);
    free(id);
    return thrd_success;
}



