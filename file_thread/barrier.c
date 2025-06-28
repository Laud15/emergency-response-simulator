#include "../file_header/barrier.h"
#include "../file_header/write_log.h"

void barrier_em_status(emergency_status_t* stato, int* count, int num_thread_barrier, mtx_t* mtx_barrier, cnd_t* cnd_barrier, int id){
    
    mtx_lock(mtx_barrier);

    (*count)++;
    if(*(count)==num_thread_barrier){

        printf("sono %d e sono arrivato per ultimo\n", *count);
        printf("ci siamo tutti!\n");
        *stato=IN_PROGRESS;//una volta che tutti sono arrivati setto lo stato dell'emergenza a IN_PROGRESS
        write_log(time(NULL), id, EMERGENCY_STATUS, "emergenza IN PROGRESS");
        *count=0;
        cnd_broadcast(cnd_barrier);
        mtx_unlock(mtx_barrier);
        
    }else{
        printf("sono arrivato %d e mi metto in attesa\n", *count);
        cnd_wait(cnd_barrier, mtx_barrier);
        mtx_unlock(mtx_barrier);
    }

}