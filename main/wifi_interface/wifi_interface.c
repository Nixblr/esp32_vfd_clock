#include "wifi_interface.h"

#include "esp_log.h"
#include "esp_wifi.h"

#include "wifiap.h"
#include "wifiSta.h"

#define TAG "WIFI Interface"

void wifiInterfaceInit(void)
{
    ESP_LOGI(TAG, "Init WiFi interface...");
    wifi_init_softap();
    wifi_init_sta();
    ESP_ERROR_CHECK(esp_wifi_start());
}