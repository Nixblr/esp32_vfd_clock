#include <esp_log.h>
#include <string.h>
#include "backend.h"
#include "cJSON.h"

static const char *TAG = "backend";
static const char* const wifiStateList[] = {"UNKNOWN", "CONNECTED", "CONNECTING", "ERR_UNKN", "ERR_AUTH"};

typedef struct WifiCfg_s
{
    char ssid[32];
    char password[32];
    char sta_ip[32];
    uint8_t state;
} wifiCfg_t;

wifiCfg_t wifiCfg = {0};

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

    if (cJSON_GetObjectItem(cjson, "cmd") != NULL)
    {
        strncpy(cmd, cJSON_GetObjectItem(cjson, "cmd")->valuestring, 64);
    }
    if (cJSON_GetObjectItem(cjson, "wifi_ssid") != NULL)
    {
        strncpy((char *) wifiCfg.ssid, cJSON_GetObjectItem(cjson, "wifi_ssid")->valuestring, 32);
    }
    if (cJSON_GetObjectItem(cjson, "wifi_pass") != NULL)
    {
        strncpy((char *) wifiCfg.password, cJSON_GetObjectItem(cjson, "wifi_pass")->valuestring, 32);
    }
    cJSON_Delete(cjson);
    ESP_LOGI(TAG, "WIFI Cfg: %s / %s", wifiCfg.ssid, wifiCfg.password);
    return 0;
}

char *backendGetStateJSON(void)
{
    char *retStr = NULL;
    const cJSON *devStateData = NULL;
    const cJSON *wifiData = NULL;

    devStateData = cJSON_CreateObject();
    if (devStateData == NULL)
    {
        return NULL;
    }
    wifiData = cJSON_CreateObject();
    if (wifiData == NULL)
    {
        cJSON_Delete(devStateData);
        return NULL;
    }
    cJSON_AddItemToObjectCS(devStateData, "wifiData", wifiData);
    if (cJSON_AddStringToObject(wifiData, "wifi_ssid", wifiCfg.ssid) == NULL)
    {
        cJSON_Delete(devStateData);
        return NULL;
    }
    if (cJSON_AddStringToObject(wifiData, "wifi_sta_ip", wifiCfg.sta_ip) == NULL)
    {
        cJSON_Delete(devStateData);
        return NULL;
    }
    if (cJSON_AddStringToObject(wifiData, "wifi_state", wifiStateList[wifiCfg.state]) == NULL)
    {
        cJSON_Delete(devStateData);
        return NULL;
    }
    retStr = cJSON_Print(devStateData);
    if (retStr == NULL)
    {
        cJSON_Delete(devStateData);
        return NULL;
    }
    return retStr;
}

void backendWiFiIPGot(char* newIP)
{
    strncpy(wifiCfg.sta_ip, newIP, 32);
}