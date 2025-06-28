#ifndef BARRIER_H
#define BARIER_H

#include "macro.h"
#include "../file_header/strutture.h"


void barrier_em_status(emergency_status_t*, int*,int, mtx_t*, cnd_t*, int);//per garantire che la barrier sia singola di ogni thread emergenza gli passo il mtx e la cnd come argomento

#endif 