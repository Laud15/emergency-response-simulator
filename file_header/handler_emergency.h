#ifndef THREAD_SERVER_H
#define THREAD_SERVE_H

#include "macro.h"
#include "strutture.h"

extern mtx_t mtx_call_rescuers;
extern cnd_t new_idle;
extern mtx_t mutex_rescuer;

int handler_emergency(void*);

#endif 
