#include "sensors.h"
#include "i2c_bus.h"
#include "sht21.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SENSORS";

static void SensorsTask(void *arg);

void SensorsInit(void)
{
    esp_err_t res;
    uint8_t sn[8];
    res = i2cBusInit();
    if (res != ESP_OK)
    {
        ESP_LOGE(TAG, "Init error: %s", esp_err_to_name(res));
    }
    Sht21Reset();
    vTaskDelay(pdMS_TO_TICKS(15));
    res = Sht21GetSerialNumber(sn);
    if (res != ESP_OK)
    {
        ESP_LOGE(TAG, "SHT21 SN read error: %s", esp_err_to_name(res));
    }
    xTaskCreatePinnedToCore(SensorsTask, "Display Task", 4096, NULL, 5, NULL, tskNO_AFFINITY);
    ESP_LOGI(TAG, "SHT21: [%x%x%x%x%x%x%x%x]", sn[0], sn[1], sn[2], sn[3], sn[4], sn[5], sn[6], sn[7]);
}

static void SensorsTask(void *arg)
{
    float rh;
    float t;
    while(1)
    {
        rh = Sht21GetHumidity();
        t = Sht21GetTemperature();
        ESP_LOGI(TAG, "T: %+2.1f H: %3.0f", t, rh);
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}