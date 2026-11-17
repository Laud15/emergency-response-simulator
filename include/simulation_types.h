#ifndef SIMULATION_TYPES_H
#define SIMULATION_TYPES_H

#include <time.h>
#include <mqueue.h>
#include <stdatomic.h>
#include <threads.h>

#define EMERGENCY_NAME_LENGTH 128
#define RESCUER_NAME_LENGTH 64
#define QUEUE_NAME_LENGTH 128

// Core simulation types based on the assignment specification.

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
    // Borrowed references to responder instances allocated by parse_rescuers.
    rescuer_digital_twin_t** rescuers_dt;
}emergency_t;

// Project-specific support types.

typedef enum{
    FILE_PARSING_ENV,
    FILE_PARSING_RESCUERS,
    FILE_PARSING_EMERGENCY_T,
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

// Parser result: processed entry count and error status (0 or -1).
typedef struct{
   int cnt;
   int error;
}parser_result_t;

typedef struct{
    int idle_count; // Available count, also used as the allocation boundary.
    rescuer_digital_twin_t* instances;
}rescuer_pool_t;

typedef struct{
    char queue_name[QUEUE_NAME_LENGTH];
    int height;
    int width;
}environment_t;

// Arguments for one responder thread; synchronization state is shared per emergency.

typedef struct {

    int id;
    int dest_x;
    int dest_y;
    int time_manage; // On-site service time in seconds.
    int* count; // Arrival count shared by this emergency's responders.
    cnd_t* cnd_barrier;
    mtx_t* mtx_barrier;
    atomic_int* n_twin; // Shared work counter, also used to claim completion.
    rescuer_digital_twin_t* twin;
    emergency_status_t* emergency_status;

}rescuer_thread_data_t;

#endif
