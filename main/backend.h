#ifndef BACKEND_H
#define BACKEND_H

#include "stdint.h"

int32_t backendProcessData(uint8_t *data);

char *backendGetStateJSON(void);

typedef enum backendWifiEvent_e
{
    BWS_STA_CONNECTED,
    BWS_STA_DISCONNECTED,
    BWS_STA_IPGOT,
    BWS_STA_AUTHERROR,
    BWS_STA_OTHERERROR,
} backendWifiEvent_t;

void backendWiFiEvent(backendWifiEvent_t e, void* arg);
void backendSetInitialStaCfg(const char *ssid, const char *pass);

#endif