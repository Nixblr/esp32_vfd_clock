#include "wifi_interface.h"

#include "esp_log.h"
#include "esp_wifi.h"

#include "wifiap.h"
#include "wifiSta.h"
#include "backend.h"

#define TAG "WIFI Interface"

void wifiInterfaceInit(void)
{
    ESP_LOGI(TAG, "Init WiFi interface...");

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    wifi_init_softap();
    wifi_init_sta();

    char ssid[32];
    char pass[64];
    wifiStaGetAP(ssid, pass);
    backendSetInitialStaCfg(ssid, pass);

    ESP_ERROR_CHECK(esp_wifi_start());
}