#include <esp_log.h>
#include <string.h>
#include "backend.h"
#include "cJSON.h"
#include "wifi_interface.h"
#include "http.h"
#include "display.h"
#include "stdbool.h"

static const char *TAG = "backend";
static const char* const wifiStateList[] = {"UNKNOWN", "WAIT_IP", "CONNECTED", "CONNECTING", "ERR_UNKN", "ERR_AUTH", "DISCONNECTED"};

static void setWiFi(cJSON *csjon);
static void setTimezone(cJSON *cjson);
static bool getWiFiStateJSON(cJSON *parent);
static bool getTimezoneJSON(cJSON *parent);

typedef struct WifiCfg_s
{
    char ssid[32];
    char password[64];
    char sta_ip[32];
    uint8_t state;
} wifiCfg_t;

wifiCfg_t wifiCfg = {0};

void backendSetInitialStaCfg(const char *ssid, const char *pass)
{
    strncpy(wifiCfg.ssid, ssid, 32);
    strncpy(wifiCfg.password, pass, 64);
}

int32_t backendProcessData(uint8_t *data)
{
    uint8_t *cmd[64];
    ESP_LOGI(TAG, "Got packet with message: %s", data);

    cJSON *cjson = cJSON_Parse((char *) data);
    /* check cJSON Parse */
    if (cjson == NULL)
    {
        ESP_LOGE(TAG, "Error: http request json parse fail");
        return -1;
    }

    if (cJSON_GetObjectItem(cjson, "wifi_ssid") != NULL)
    {
        setWiFi(cjson);
    }

    if (cJSON_GetObjectItem(cjson, "timezone") != NULL)
    {
        setTimezone(cjson);
    }
    cJSON_Delete(cjson);
    return 0;
}

static void setWiFi(cJSON *cjson)
{
    if (cJSON_GetObjectItem(cjson, "wifi_ssid") != NULL)
    {
        strncpy(wifiCfg.ssid, cJSON_GetObjectItem(cjson, "wifi_ssid")->valuestring, 32);
    }
    if (cJSON_GetObjectItem(cjson, "wifi_pass") != NULL)
    {
        strncpy(wifiCfg.password, cJSON_GetObjectItem(cjson, "wifi_pass")->valuestring, 64);
    }
    ESP_LOGI(TAG, "WIFI Cfg: %s / %s", wifiCfg.ssid, wifiCfg.password);
    wifiStaChangeAP(wifiCfg.ssid, wifiCfg.password);
}

static void setTimezone(cJSON *cjson)
{
    int tz;
    if (cJSON_IsNumber(cJSON_GetObjectItem(cjson, "timezone")))
    {
        tz = cJSON_GetObjectItem(cjson, "timezone")->valueint;
        if (tz != DisplayGetTimezone())
        {
            DisplaySetTimezone(tz);
            webInterfaceUpdateToClients(BR_TIMEZONE);
        }
    }
}

char *backendGetStateJSON(backendRequest_t rd)
{
    char *retStr = NULL;
    const cJSON *devStateData = NULL;

    devStateData = cJSON_CreateObject();
    if (devStateData == NULL)
    {
        return NULL;
    }
    if (rd == BR_WIFI || rd == BR_FULL)
    {
        if (getWiFiStateJSON(devStateData) == false)
        {
            cJSON_Delete(devStateData);
            return NULL;
        }
    }

    if (rd == BR_TIMEZONE || rd == BR_FULL)
    {
        if (getTimezoneJSON(devStateData) == false)
        {
            cJSON_Delete(devStateData);
            return NULL;
        }
    }

    retStr = cJSON_Print(devStateData);
    if (retStr == NULL)
    {
        cJSON_Delete(devStateData);
        return NULL;
    }
    return retStr;
}

static bool getWiFiStateJSON(cJSON *parent)
{
    const cJSON *wifiData = NULL;
    wifiData = cJSON_CreateObject();
    if (wifiData == NULL)
    {
        return false;
    }
    if (cJSON_AddStringToObject(wifiData, "wifi_ssid", wifiCfg.ssid) == NULL)
    {
        cJSON_Delete(wifiData);
        return false;
    }
    if (cJSON_AddStringToObject(wifiData, "wifi_sta_ip", wifiCfg.sta_ip) == NULL)
    {
        cJSON_Delete(wifiData);
        return false;
    }
    if (cJSON_AddStringToObject(wifiData, "wifi_state", wifiStateList[wifiCfg.state]) == NULL)
    {
        cJSON_Delete(wifiData);
        return false;
    }
    cJSON_AddItemToObjectCS(parent, "wifiData", wifiData);
    return true;
}

static bool getTimezoneJSON(cJSON *parent)
{
    if (cJSON_AddNumberToObject(parent, "timezone", DisplayGetTimezone()) == NULL)
    {
        return false;
    }
    return true;
}

void backendWiFiEvent(backendWifiEvent_t e, void* arg)
{
    switch (e)
    {
    case BWS_STA_CONNECTED:
        wifiCfg.state = 1;
        break;
    case BWS_STA_IPGOT:
        wifiCfg.state = 2;
        strncpy(wifiCfg.sta_ip, (char *)arg, 32);
        break;

    case BWS_STA_DISCONNECTED:
        wifiCfg.state = 6;
        wifiCfg.sta_ip[0] = '\0';
        break;

    case BWS_STA_AUTHERROR:
        wifiCfg.state = 5;
        break;

    case BWS_STA_OTHERERROR:
        wifiCfg.state = 4;
        break;

    case BWS_STA_CONNECTING:
        wifiCfg.state = 3;
        break;

    default:
        wifiCfg.state = 0;
        break;
    }
    webInterfaceUpdateToClients(BR_WIFI);
}