#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "string.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "backend.h"
#include "display.h"

#define TAG "WIFI STA"

static int s_retry_num = 0;

static void wifi_sta_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    /**
     * STA Event
    */
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "<STA> WIFI_EVENT_STA_START");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        backendWiFiEvent(BWS_STA_CONNECTED, NULL);
        DisplayShowMessage("ConnEctEd", DSE_NONE, 1000);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        backendWiFiEvent(BWS_STA_DISCONNECTED, NULL);
        DisplayShowMessage("dISconn", DSE_NONE, 1000);
        if (s_retry_num < 3) {
            if (!s_retry_num)
            {
                ESP_LOGI(TAG, "<STA> WIFI_EVENT_STA_DISCONNECTED");
            }
            ESP_LOGI(TAG, "<STA> Try to reconnect 1...");
            esp_wifi_connect();
            s_retry_num++;
        } else 
        {
            ESP_LOGI(TAG, "<STA> Failed to connect...");
            DisplayShowMessage("con Error", DSE_NONE, 1000);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        char ipstr[32];
        sprintf(ipstr, IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "got ip: %s", ipstr); 
        backendWiFiEvent(BWS_STA_IPGOT, ipstr);
        DisplayShowMessage("IP GOt", DSE_NONE, 1000);
        s_retry_num = 0;
    }
    else
    {
        ESP_LOGI(TAG, "<EVENT> Base: %d; Id: %d", (int) event_base, (int) event_id);
    }
}

void wifi_init_sta(void)
{
    esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_sta_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_sta_event_handler,
                                                        NULL,
                                                        NULL));
    /*wifi_config_t wifi_config_sta = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta));*/
}

void wifiStaChangeAP(const char *ssid, const char *pass)
{
    wifi_config_t wifi_config_sta = {0};
    wifi_config_sta.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    strncpy((char *) wifi_config_sta.sta.ssid, ssid, 32);
    strncpy((char *) wifi_config_sta.sta.password, pass, 64);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta));
    esp_wifi_disconnect();
}

void wifiStaGetAP(char *ssid, char *pass)
{
    wifi_config_t wifiCfg;
    esp_wifi_get_config(WIFI_IF_STA, &wifiCfg);
    strncpy(ssid, (char *) wifiCfg.sta.ssid, 32);
    strncpy(pass, (char *) wifiCfg.sta.password, 64);
}