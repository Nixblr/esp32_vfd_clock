#include "wifiap.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_system.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "string.h"
#include "display.h"

#define VFD_CLOCK_WIFI_SSID      "VFDClock"
#define VFD_CLOCK_WIFI_PASS      ""
#define VFD_CLOCK_WIFI_CHANNEL   1
#define VFD_CLOCK_MAX_STA_CONN   2

static const char *TAG = "wifi softAP";

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        //ESP_LOGI(TAG, "station \"MACSTR\" join, AID=%d",
        //         MAC2STR(event->mac), event->aid);
        DisplayShowMessage("St JoInEd", DSE_NONE, 1000);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        //ESP_LOGI(TAG, "station \"MACSTR\" leave, AID=%d",
        //         MAC2STR(event->mac), event->aid);
        DisplayShowMessage("St LEAVE", DSE_NONE, 1000);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        DisplayShowMessage("AP rEAdY", DSE_NONE, 1000);
    }
}

void wifi_init_softap(void)
{
    esp_netif_create_default_wifi_ap();
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        WIFI_EVENT_AP_STACONNECTED,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        WIFI_EVENT_AP_STADISCONNECTED,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        WIFI_EVENT_AP_START,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config_ap = {
        .ap = {
            .ssid = VFD_CLOCK_WIFI_SSID,
            .ssid_len = strlen(VFD_CLOCK_WIFI_SSID),
            .channel = VFD_CLOCK_WIFI_CHANNEL,
            .password = VFD_CLOCK_WIFI_PASS,
            .max_connection = VFD_CLOCK_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(VFD_CLOCK_WIFI_PASS) == 0) {
        wifi_config_ap.ap.authmode = WIFI_AUTH_OPEN;
    }
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap));
    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             VFD_CLOCK_WIFI_SSID, VFD_CLOCK_WIFI_PASS, VFD_CLOCK_WIFI_CHANNEL);
}
