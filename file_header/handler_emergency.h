#ifndef THREAD_SERVER_H
#define THREAD_SERVE_H

#include "macro.h"
#include "strutture.h"

extern cnd_t new_idle_pri2;
extern cnd_t new_idle_pri1;
extern cnd_t new_idle_pri0;
extern mtx_t mtx_call_rescuers;

extern rescuer_pool_t* rescuer_pools;
extern int rescuer_pool_count;

extern emergency_type_t* emergency_types;
extern int emergency_type_count;

extern environment_t environment;
extern mqd_t mq;

extern atomic_int go;

int handler_emergency(void*);

#endif 
