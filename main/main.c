#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "display.h"
#include "wifi_interface.h"
#include <string.h>
#include "http.h"
#include "nvs_flash.h"
#include "sensors.h"


void app_main(void)
{
    uint8_t dat[11];
    static uint8_t i = 0;

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    DisplayInit();
    SensorsInit();
    wifiInterfaceInit();
    httpap_init();
    while (1){
        vTaskDelay(pdMS_TO_TICKS(1000));
    };
}
