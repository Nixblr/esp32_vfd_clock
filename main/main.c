#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "display.h"
#include "wifiap.h"
#include <string.h>
#include "http.h"



void app_main(void)
{
    uint8_t dat[11];
    static uint8_t i = 0;
    DisplayInit();
    wifiap_create();
    httpap_init();
    while (1){
        vTaskDelay(pdMS_TO_TICKS(1000));
    };
}
