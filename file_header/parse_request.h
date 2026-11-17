#ifndef PARSE_REQUEST_H
#define PARSE_REQUEST_H

#include "macro.h"
#include "strutture.h"

// Read a request file and send valid rows sequentially after their delays.
parser_result_t parse_and_send(char*, mqd_t);

// Parse name and coordinates, returning the delay; the caller sets the timestamp.
// On a malformed row, set the name to "Errore" and return zero.
int parse_request_line(char*, emergency_request_t*);

#endif