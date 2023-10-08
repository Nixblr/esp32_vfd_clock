#ifndef BACKEND_H
#define BACKEND_H

#include "stdint.h"

int32_t backendProcessData(uint8_t *data);

char *backendGetStateJSON(void);

void backendWiFiIPGot(char* newIP);

#endif