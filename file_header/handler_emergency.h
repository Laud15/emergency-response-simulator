#ifndef THREAD_SERVER_H
#define THREAD_SERVE_H

#include "macro.h"
#include "strutture.h"

extern cnd_t new_idle_pri2;
extern cnd_t new_idle_pri1;
extern cnd_t new_idle_pri0;
extern mtx_t mtx_call_rescuers;

extern coppie_numero_gemelli* lista_rescuers;
extern int len_lst_rescuers;

extern emergency_type_t* lista_emergency_type;
extern int len_lst_emergency_t;

extern env ambiente;
extern mqd_t mq;

extern atomic_int go;

int handler_emergency(void*);

#endif 
