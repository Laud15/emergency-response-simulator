#ifndef THREAD_SERVER_H
#define THREAD_SERVE_H

#include "macro.h"
#include "strutture.h"

extern cnd_t new_idle_pri2;
extern cnd_t new_idle_pri1;
extern cnd_t new_idle_pri0;

extern mtx_t mtx_call_rescuers;
extern mtx_t mutex_rescuer;

int handler_emergency(void*);

#endif 
