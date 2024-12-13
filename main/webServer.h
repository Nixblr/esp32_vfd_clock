#ifndef HTTP_H
#define HTTP_H

#include "esp_err.h"
#include "backend.h"

void httpap_init(void);
esp_err_t webInterfaceUpdateToClients(backendRequest_t content);

#endif