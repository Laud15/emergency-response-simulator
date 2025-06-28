#ifndef STRUTTURE_H
#define STRUTTURE_H

#include <time.h>
#include <mqueue.h>
#include <stdatomic.h>
#include <threads.h>

#define EMERGENCY_NAME_LENGTH 128
#define RESCUER_NAME_LENGTH 64
#define QUEUE_NAME_LENGTH 128

//STRUTTURE RICHIESTE DALLA CONSEGNA

typedef enum{
    WAITING, 
    ASSIGNED, 
    IN_PROGRESS, 
    PAUSED, 
    COMPLETED,
    CANCELED, 
    TIMEOUT
}emergency_status_t;

typedef enum {
    IDLE, 
    EN_ROUTE_TO_SCENE, 
    ON_SCENE, 
    RETURNING_TO_BASE
} rescuer_status_t;

typedef struct{
    char *      rescuer_type_name;
    int         speed;
    int         x;
    int         y;
}rescuer_type_t;

typedef struct{
    int                 id;
    int                 x;
    int                 y;
    rescuer_type_t *    rescuer;
    rescuer_status_t    status;
}rescuer_digital_twin_t;


typedef struct {
    rescuer_type_t *    type;
    int                 required_count;
    int                 time_to_manage;
}rescuer_request_t;

typedef struct {
    short                   priority;
    char *                  emergency_desc;
    rescuer_request_t *     rescuers;
    int                     rescuers_req_number;
}emergency_type_t;

typedef struct {
    char    emergency_name[EMERGENCY_NAME_LENGTH];
    int     x;
    int     y;
    time_t  timestamp;
}emergency_request_t;

typedef struct {
    emergency_type_t        type;
    emergency_status_t      status;
    int                     x;
    int                     y;
    time_t                  time;
    int                     rescuer_count;
    rescuer_digital_twin_t** rescuers_dt; //cambiato da quello  originale per far si di poter contenere il riferimento ai puntatori dei gemelli allocati inizialmente con il parse
}emergency_t;

//-----------------STRUTTURE FATTE DA ME-----------------


typedef enum{
    FILE_PARSING_ENV,
    FILE_PARSING_RESCUERS,
    FILE_PARSING_EMERGENCY_T,
    SHM,
    MESSAGE_QUEUE,
    EMERGENCY_STATUS,
    RESCUER_STATUS,
    RESCUER_ASSIGNMENT,
    TIMEOUT_FOR_DISTANCE,
    TIMEOUT_FOR_RESOURCE,
    INVALID_INPUT,
    CLIENT_REQUEST,
    SHUTDOWN
}log_event;


typedef struct{
   int cnt;
   int error;
}res_parser;//definisco la struttura di ritorno dei parser, error viene no sempre inizializzato a 0 e rimane a 0 se non si verifica nessun errore, altrimenti viene messo a -1

typedef struct{
    int n;
    rescuer_digital_twin_t* lst_gemelli;
}coppie_numero_gemelli; //definisco una struttura con due campi, il primo un intero e il secondo un puntatore a rescuer_digital_twin_t

typedef struct{
    char nome[EMERGENCY_NAME_LENGTH];
    short prio;
}coppia_emergenza_priorità;


typedef struct{
    char nome_msg_queue[QUEUE_NAME_LENGTH];
    int height;
    int width;
}env;

//Strutture usate per i thread

typedef struct{ 

    char* riga;
    int (*f)(char*, emergency_request_t*);//funzione di che fara il parsing della liga
    int len_em_pri;
    mqd_t mq;
    coppia_emergenza_priorità* list_em_pri;//lista da dove prelevare la priorità

}data_client_parse;//utilizzo questa funzione per passare al thread una riga da parsare, un parser e un posto dove mettere la riga parsata


typedef struct{
    int id;//id dell'emergenza
    coppie_numero_gemelli* lista_soccorritori;//lista contente tutti i gemelli parsati all'avvio del server
    emergency_type_t* lista_tipi_emergenze;//lista contenente tutti i tipi di emergenza parsati all'avvio del server
    env amb;//struttura contente larghezza, altezza e nome della mq
    mqd_t* mq;//puntatore alla mq aperta
    int  len_lst_socc;//lunghezza lista soccorritori
    int len_lst_em;//lunghezza lista emergenze
}data_server_thread;


typedef struct {

    int id;//id della richiesta
    int dest_x;
    int dest_y;//coordinate di destinazione
    int time_manage;//per quanto tempo il rescuer dovrà fermarsi sul luogo dell'emergenza
    int* count;//contatre comune a tutti i thread soccorritori di un emergenza, serve per sincronizzarsi nella barrier
    cnd_t* cnd_barrier;//
    mtx_t* mtx_barrier;//elementi usati per la sincronizzazione(ogni thread emergenza ne ha una copia diversa)
    atomic_int* n_twin;//numero di gemelli richiesti dall'emergenza, questa variabile è condivisa da tutti i thread soccorritori creati da un'emergenza
    rescuer_digital_twin_t* twin;//puntatore al soccorritore (ovvero uno dei soccorritori chiamati da call_rescuers)
    emergency_status_t* stato_emergenza;//puntatore allo stato dell'emergenza chiamata

}data_rescuer_thread;


#endif
