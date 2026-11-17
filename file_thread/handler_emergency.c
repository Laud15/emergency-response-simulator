#include "../file_header/handler_emergency.h"
#include "../file_header/barrier.h"
#include "../file_header/write_log.h"

#define TIME_PRI1 30
#define TIME_PRI2 10
#define TIME_OUT 0
#define IN_TIME 1
#define NOT_ALL_RESCUERS 0
#define ALL_RESCUERS 1

// Waiting-emergency counters are accessed under mtx_call_rescuers.
int pri2_cnt=0;
int pri1_cnt=0;

int count_required_rescuers(emergency_type_t emergency_type){
    int cnt=0;

    for(size_t i=0; i<emergency_type.rescuers_req_number;i++){
        printf("Required: %d %s responders\n", emergency_type.rescuers[i].required_count, emergency_type.rescuers[i].type->rescuer_type_name);
        cnt+=emergency_type.rescuers[i].required_count;
    }

    return cnt;
}

// Check the slowest base-to-scene travel estimate against the remaining deadline.
int can_arrive_before_deadline(emergency_type_t emergency, int x_dest, int y_dest, struct timespec deadline, struct timespec start){

    printf("\nChecking arrival time for %s <x:%d, y:%d>\n", emergency.emergency_desc, x_dest, y_dest);
    int max=0;
    int travel_time;
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    // Elapsed seconds since the client-side request timestamp.
    int sec = (int)(now.tv_sec-start.tv_sec);

    printf("\nElapsed time for %s <x:%d, y:%d>: %d seconds\n",emergency.emergency_desc, x_dest, y_dest, sec);

    // Priority 0 bypasses arrival deadlines.
    if(emergency.priority>0){
        if(sec>=(deadline.tv_sec-start.tv_sec)){
            return TIME_OUT;
        }

        for(size_t i=0; i<emergency.rescuers_req_number;i++){

            travel_time=TRAVEL_TIME(emergency.rescuers[i].type->speed, emergency.rescuers[i].type->x, emergency.rescuers[i].type->y, x_dest, y_dest);

            if(travel_time>max){
                max=travel_time;
            }
        }

        if(max>(deadline.tv_sec-now.tv_sec)){
            printf("Maximum arrival time for %s <x:%d, y:%d>: %d seconds exceeds the remaining time\n",emergency.emergency_desc, x_dest, y_dest, max);
            return TIME_OUT;
        }
        printf("Arrival time for %s <x:%d,y:%d>: %d seconds is within the deadline\n\n", emergency.emergency_desc, x_dest, y_dest, max);
        return IN_TIME;
    }

    printf("No arrival deadline for %s <x:%d,y:%d>\n", emergency.emergency_desc, x_dest, y_dest);
    return IN_TIME;
    
}

int rescuer_thread(void* arg){

    rescuer_thread_data_t* resc_data = (rescuer_thread_data_t*) arg;
    int travel_time=TRAVEL_TIME(resc_data->twin->rescuer->speed, resc_data->twin->rescuer->x,resc_data->twin->rescuer->y, resc_data->dest_x, resc_data->dest_y);
    int expected=0;
    int desired=1;
    char buffer[BUFFERSIZE];

    resc_data->twin->status=EN_ROUTE_TO_SCENE;
    snprintf(buffer, BUFFERSIZE, "Rescuer id: %d %s set to EN_ROUTE_TO_SCENE", resc_data->twin->id ,resc_data->twin->rescuer->rescuer_type_name);
    write_log(time(NULL), resc_data->id, RESCUER_ASSIGNMENT, buffer);

    sleep(travel_time);

    resc_data->twin->status=ON_SCENE;
    snprintf(buffer, BUFFERSIZE, "Rescuer id: %d %s set to ON_SCENE", resc_data->twin->id ,resc_data->twin->rescuer->rescuer_type_name);
    write_log(time(NULL), resc_data->id, RESCUER_STATUS, buffer);
    
    // Synchronize arrivals before starting on-site work.
    barrier_em_status(resc_data->emergency_status, resc_data->count, (int)(*resc_data->n_twin), resc_data->mtx_barrier, resc_data->cnd_barrier, resc_data->id);

    sleep(resc_data->time_manage);
    printf("Rescuer %d %s finished on-site work\n", resc_data->twin->id,resc_data->twin->rescuer->rescuer_type_name);

    // Only the thread that changes the counter from zero to one records completion.
    atomic_fetch_sub(resc_data->n_twin, 1);
    if(atomic_compare_exchange_strong(resc_data->n_twin,&expected, desired)){
        printf("\n--Emergency at <x:%d, y:%d> completed--\n\n", resc_data->dest_x, resc_data->dest_y);
        (*(resc_data->emergency_status))=COMPLETED;
        write_log(time(NULL), resc_data->id, EMERGENCY_STATUS, "emergenza COMPLETED");
    }
    
    resc_data->twin->status=RETURNING_TO_BASE;
    snprintf(buffer, BUFFERSIZE, "Rescuer id: %d %s set to RETURNING TO BASE", resc_data->twin->id ,resc_data->twin->rescuer->rescuer_type_name);
    write_log(time(NULL), resc_data->id, RESCUER_STATUS, buffer);

    sleep(travel_time);
    resc_data->twin->status=IDLE;

    snprintf(buffer, BUFFERSIZE, "Rescuer id: %d %s returned to base, set to IDLE", resc_data->twin->id ,resc_data->twin->rescuer->rescuer_type_name);
    write_log(time(NULL), resc_data->id, RESCUER_STATUS, buffer);

    // Restore availability under the allocation lock, then wake the highest waiting priority.
    THRD_CHECK(mtx_lock(&mtx_call_rescuers), "errore nella lock di mtx_call_rescuer");

    for(size_t i=0; i<rescuer_pool_count;i++){
        if(strcmp(resc_data->twin->rescuer->rescuer_type_name, rescuer_pools[i].instances[0].rescuer->rescuer_type_name)==STRCMP_TRUE){
            rescuer_pools[i].idle_count++;
            break;
        }
    }

    if(pri2_cnt>0){
        printf("Waking priority-2 requests\n");
        THRD_CHECK(cnd_broadcast(&new_idle_pri2), "errore nella broadcast di new_idle");

    }else if(pri1_cnt>0){
        printf("Waking priority-1 requests\n");
        THRD_CHECK(cnd_broadcast(&new_idle_pri1), "errore nella broadcast di new_idle");
        
    }else{
        printf("Waking priority-0 requests\n");
        THRD_CHECK(cnd_broadcast(&new_idle_pri0), "errore nella broadcast di new_idle");
    }

    THRD_CHECK(mtx_unlock(&mtx_call_rescuers), "errore nella unlock di mtx_call_rescuer");

    free(resc_data);
    return thrd_success;
}

// Launch one thread per assigned responder and join them after return to base.
int solve(emergency_t* emergency, int id){
    
    thrd_t rescuers[emergency->rescuer_count];
    atomic_int n_twin=(atomic_int)emergency->rescuer_count;
    // Each emergency owns its arrival barrier and shares it among its responders.
    mtx_t mtx_barrier;
    cnd_t cnd_barrier;
    int count_barrier=0;

    THRD_CHECK(mtx_init(&mtx_barrier, mtx_plain), "errore nella init di mtx_barrier");
    THRD_CHECK(cnd_init(&cnd_barrier), "errore nella init di cnd_barrier");
    
    for(size_t i=0; i<emergency->rescuer_count;i++){

        rescuer_thread_data_t* data;
        SNCALL(data, malloc(sizeof(rescuer_thread_data_t)), "errore nell'allocazione di memoria per i thread soccorritori");

        data->id=id;
        data->twin=emergency->rescuers_dt[i];
        data->dest_x=emergency->x;
        data->dest_y=emergency->y;
        data->n_twin=&n_twin;
        data->emergency_status=&(emergency->status);
        data->cnd_barrier=&cnd_barrier;
        data->mtx_barrier=&mtx_barrier;
        data->count=&count_barrier;
       
        for(size_t j=0; j<emergency->type.rescuers_req_number;j++){
            if(strcmp(data->twin->rescuer->rescuer_type_name, emergency->type.rescuers[j].type->rescuer_type_name)==STRCMP_TRUE){
                data->time_manage=emergency->type.rescuers[j].time_to_manage;
            }
        }
        THRD_CHECK(thrd_create(&rescuers[i], rescuer_thread, data), "errore nella create dei rescuer thread");
    }

    for(size_t i=0; i<emergency->rescuer_count;i++){
        THRD_CHECK(thrd_join(rescuers[i], NULL), "errore nella join dei rescuer_thread");
    }
    
    mtx_destroy(&mtx_barrier);
    cnd_destroy(&cnd_barrier);
    return 0;
}

int call_rescuers(emergency_t* emergency){

    int current_position=0;
    // Check every requested type before reserving any responders.
    for(size_t i=0;i<emergency->type.rescuers_req_number;i++){
        for(size_t j=0;j<rescuer_pool_count;j++){

            if(strcmp(emergency->type.rescuers[i].type->rescuer_type_name, rescuer_pools[j].instances[0].rescuer->rescuer_type_name)==STRCMP_TRUE){
                if(emergency->type.rescuers[i].required_count>rescuer_pools[j].idle_count){
                    return NOT_ALL_RESCUERS;
                }
            }
        }
    }
    // Allocate backwards from each pool's idle-count boundary; states are not checked.
    for(size_t i=0; i<emergency->type.rescuers_req_number;i++){
        for(size_t j=0;j<rescuer_pool_count;j++){

            if(strcmp(emergency->type.rescuers[i].type->rescuer_type_name, rescuer_pools[j].instances[0].rescuer->rescuer_type_name)==STRCMP_TRUE){

                for(size_t k=0; k<emergency->type.rescuers[i].required_count;k++){
                    emergency->rescuers_dt[current_position]=&(rescuer_pools[j].instances[(rescuer_pools[j].idle_count-1)]);
                    printf("Assigned <id:%d type:%s> to slot %d\n", rescuer_pools[j].instances[(rescuer_pools[j].idle_count-1)].id, rescuer_pools[j].instances[(rescuer_pools[j].idle_count-1)].rescuer->rescuer_type_name, current_position);
                    rescuer_pools[j].idle_count--;
                    current_position++;
                }
            }
        }
    }
    return ALL_RESCUERS;
}

// Reserve all required responders, waiting for availability, then run the simulation.
int try_solve(emergency_t* emergency, int id){

    printf("Attempting to handle %s\n", emergency->type.emergency_desc);

    struct timespec deadline;
    struct timespec start;
    int cnd_res;

    // Compute wait deadlines from the current time; start holds the request timestamp.
    start.tv_nsec=0;
    start.tv_sec=emergency->time;
    timespec_get(&deadline, TIME_UTC);

    switch (emergency->type.priority){

        case PRI_0:
            break;
        case PRI_1:
            deadline.tv_nsec=0;
            deadline.tv_sec+=TIME_PRI1;
            break;
        case PRI_2:
            deadline.tv_nsec=0;
            deadline.tv_sec+=TIME_PRI2;
            break;
        default:
        fprintf(stderr, "Unknown priority");
        return thrd_error;

    }
    
    // Reject requests whose estimated arrival exceeds the remaining deadline.
    if(emergency->type.priority>0){
        if(can_arrive_before_deadline(emergency->type,emergency->x, emergency->y, deadline, start)==TIME_OUT){

            printf("\n--Emergency %s TIMEOUT--\n\n", emergency->type.emergency_desc);
            emergency->status=TIMEOUT;
            write_log(time(NULL), id, TIMEOUT_FOR_DISTANCE, "TIMEOUT dell'emergenza");
            return thrd_success;

        }
    }

    // Serialize resource checks and reservations across emergency handlers.
    THRD_CHECK(mtx_lock(&mtx_call_rescuers), "errore nella lock di mtx_call_rescuers");
    
    while(call_rescuers(emergency)==NOT_ALL_RESCUERS){

        emergency->status=PAUSED;
        write_log(time(NULL), id, EMERGENCY_STATUS, "emergenza messa in PAUSE");

        if(emergency->type.priority>PRI_0){

                if(emergency->type.priority==PRI_2){
                    pri2_cnt++;
                    printf("\n---Waiting for responders: %d priority-2 requests paused---\n", pri2_cnt);
                    cnd_res = cnd_timedwait(&new_idle_pri2, &mtx_call_rescuers, &deadline);
                }else{
                    pri1_cnt++;
                    printf("\n---Waiting for responders: %d priority-1 requests paused---\n", pri1_cnt);
                    cnd_res = cnd_timedwait(&new_idle_pri1, &mtx_call_rescuers, &deadline);
                }

                THRD_CHECK(cnd_res, "errore nella cnd_timedwait");

                printf("\n%s <x:%d,y:%d> notified of available responders\n\n", emergency->type.emergency_desc, emergency->x, emergency->y);
                emergency->status=WAITING;
                write_log(time(NULL), id, EMERGENCY_STATUS, "emergenza messa in WAITING");

                if(emergency->type.priority==PRI_2){
                    pri2_cnt--;
                    printf("\n---Priority-2 requests still waiting: %d---\n", pri2_cnt);
                }else{
                    pri1_cnt--;
                    printf("\n---Priority-1 requests still waiting: %d---\n", pri1_cnt);
                }

                // Stop if the wait expired or the remaining time no longer permits arrival.
                if(cnd_res == thrd_timedout || (can_arrive_before_deadline(emergency->type,emergency->x, emergency->y, deadline, start)==TIME_OUT)){

                    printf("\n%s <x:%d, y:%d> timed out while waiting\n\n", emergency->type.emergency_desc, emergency->x, emergency->y);
                    emergency->status=TIMEOUT;
                    write_log(time(NULL), id, TIMEOUT_FOR_RESOURCE, "TIMEOUT dell'emergenza");
                    mtx_unlock(&mtx_call_rescuers);

                    return 0;
                }

            }else{
                // Priority 0 waits for availability without a deadline.
                THRD_CHECK(cnd_wait(&new_idle_pri0, &mtx_call_rescuers), "errore nella cnd_wait di new_idle");
                printf("\n%s <x:%d,y:%d> notified of available responders\n\n", emergency->type.emergency_desc, emergency->x, emergency->y);
                emergency->status=WAITING;
                write_log(time(NULL), id, EMERGENCY_STATUS, "emergenza messa in WAITING");
            }
    }

    printf("\n--All required responders are available--\n\n");
    for(size_t i=0;i<emergency->rescuer_count;i++){
        emergency->rescuers_dt[i]->status=EN_ROUTE_TO_SCENE;
    }

    THRD_CHECK(mtx_unlock(&mtx_call_rescuers), "errore nella unlock di mtx_call_rescuers");

    emergency->status=ASSIGNED;
    write_log(time(NULL), id, EMERGENCY_STATUS, "Emergenza messa in ASSIGNED");

    solve(emergency, id);

    return 1;
}

// Build an emergency instance from a known type and valid grid coordinates.
emergency_t create_emergency_instance(emergency_request_t emergency, int id){

    emergency_t emergency_instance;
    int type_found=0;
    emergency_instance.rescuers_dt=NULL;

    // Coordinates are one-based and bounded by the configured grid.
    if(POS_AND_MIN(emergency.x, environment.width) && POS_AND_MIN(emergency.y, environment.height)){

            for(size_t i=0;i<emergency_type_count;i++){

                if(strcmp(emergency.emergency_name, emergency_types[i].emergency_desc)==STRCMP_TRUE){

                    emergency_instance.type=emergency_types[i];
                    emergency_instance.status=WAITING;
                    emergency_instance.x=emergency.x;
                    emergency_instance.y=emergency.y;
                    emergency_instance.time=emergency.timestamp;
                    emergency_instance.rescuer_count=count_required_rescuers(emergency_types[i]);
                    SNCALL(emergency_instance.rescuers_dt, malloc(sizeof(rescuer_digital_twin_t*)*emergency_instance.rescuer_count), "errore nell'allocazione di memoria del campo rescuers_dt");

                    write_log(time(NULL), id, MESSAGE_QUEUE, "richiesta elaborata correttamente");
                    write_log(time(NULL), id, EMERGENCY_STATUS, "emergenza messa in stato di WAIT");
                    type_found=1;
                    break;
                }
            }

            // Cancel requests with an unknown emergency name.
            if(!type_found){
                write_log(time(NULL), id, MESSAGE_QUEUE, "richiesta emergenza non esistente");
                emergency_instance.status=CANCELED;
                write_log(time(NULL), id, EMERGENCY_STATUS, "richiesta CANCELED");
                return emergency_instance;
            }

        return emergency_instance;

    }else{
        emergency_instance.status=CANCELED;
        write_log(time(NULL), id, MESSAGE_QUEUE, "coordinate errate inserite");
        write_log(time(NULL), id, EMERGENCY_STATUS, "richiesta CANCELED");
        return emergency_instance;
    }
}

int handler_emergency(void* arg){

    char buffer[BUFFERSIZE];
    int* id = (int*) arg;
    emergency_request_t req;
    
    if(mq_receive(mq, (char*)&req, sizeof(emergency_request_t), NULL) == SCALL_ERROR){
        write_log(time(NULL), *id, MESSAGE_QUEUE, "errore nella lettura della mq");
        perror("mq_receive");
        free(id);
        return thrd_error;
    }

    // Stop this handler; the server still joins all other handlers in the batch.
    if(strcmp(req.emergency_name,"shutdown_server")==STRCMP_TRUE){
        write_log(time(NULL), *id, SHUTDOWN, "richiesta di shutdown server");
        free(id);
        atomic_store(&go, 0);
        return thrd_success;
    }
    
    snprintf(buffer, BUFFERSIZE, "Received emergency: %s at <x:%d,y:%d>", req.emergency_name, req.x, req.y);
    write_log(time(NULL), *id, MESSAGE_QUEUE, buffer);

    emergency_t emergency = create_emergency_instance(req, *id);
    
    if(emergency.status!=CANCELED){
        printf("Handler %d processing %s with priority %hd at <x:%d, y:%d>\n", *id, emergency.type.emergency_desc,emergency.type.priority, emergency.x, emergency.y);
        try_solve(&emergency, *id);
    }else{
        printf("Handler %d received an invalid emergency\n", *id);
    }

    free(emergency.rescuers_dt);
    free(id);
    return thrd_success;
}

