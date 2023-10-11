#ifndef BACKEND_H
#define BACKEND_H

#include "stdint.h"

typedef enum backendWifiEvent_e
{
    BWS_STA_CONNECTING,
    BWS_STA_CONNECTED,
    BWS_STA_DISCONNECTED,
    BWS_STA_IPGOT,
    BWS_STA_AUTHERROR,
    BWS_STA_OTHERERROR,
} backendWifiEvent_t;

typedef enum backendRequest_e
{
    BR_FULL,
    BR_WIFI,
    BR_TIMEZONE,
    BR_DISP_STATE
} backendRequest_t;

int32_t backendProcessData(uint8_t *data);
char *backendGetStateJSON(backendRequest_t rd);
void backendWiFiEvent(backendWifiEvent_t e, void* arg);
void backendSetInitialStaCfg(const char *ssid, const char *pass);

#endif