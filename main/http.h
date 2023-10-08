#ifndef HTTP_H
#define HTTP_H

#include "esp_err.h"

void httpap_init(void);
esp_err_t webInterfaceUpdateToClients(void);

#endif