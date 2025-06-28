#ifndef PARSE_REQUEST_H
#define PARSE_REQUEST_H

#include "macro.h"
#include "strutture.h"

res_parser parse_file(char*, mqd_t, coppia_emergenza_priorità*, int);//funzione che prende in input un puntatore ad un puntatore di una lista di puntatori a stringhe e un path di file, la funzione scorre tutte le righe del filee le mette nella lista

int parse_riga(char*, emergency_request_t*);//presa una riga e un puntatore a emergency_request t, legge la riga, assegna il nome, x, y letti all'emergneza e ne setta il timestamp e restituisce il delay

#endif