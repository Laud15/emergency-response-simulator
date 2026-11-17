#ifndef BARRIER_H
#define BARIER_H

#include "macro.h"
#include "../include/simulation_types.h"

// The counter, mutex and condition variable belong to one emergency.
void barrier_em_status(emergency_status_t*, int*,int, mtx_t*, cnd_t*, int);

#endif 