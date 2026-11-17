#include "../include/barrier.h"
#include "../include/write_log.h"

void barrier_em_status(emergency_status_t* status, int* count, int num_thread_barrier, mtx_t* mtx_barrier, cnd_t* cnd_barrier, int id){
    
    mtx_lock(mtx_barrier);

    (*count)++;
    // The last arrival marks the emergency in progress and wakes the others.
    if(*(count)==num_thread_barrier){

        printf("All responders have arrived\n");
        *status=IN_PROGRESS;
        write_log(time(NULL), id, EMERGENCY_STATUS, "emergenza IN PROGRESS");
        *count=0;
        cnd_broadcast(cnd_barrier);
        mtx_unlock(mtx_barrier);
        
    }else{
        cnd_wait(cnd_barrier, mtx_barrier);
        mtx_unlock(mtx_barrier);
    }

}