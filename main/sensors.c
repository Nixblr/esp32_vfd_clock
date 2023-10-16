#include "sensors.h"
#include "i2c_bus.h"
#include "sht21.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "bmp085.h"

static const char *TAG = "SENSORS";
static bmp085_t bmp085;
static SemaphoreHandle_t accessMutex = NULL;

static void SensorsTask(void *arg);
static void delayMs(uint32_t d);
static void initDevices(void);

static void delayMs(uint32_t d)
{
    //vTaskDelay(pdMS_TO_TICKS(d));
    vTaskDelay(pdMS_TO_TICKS(50));  //Incorrect delay in bmp085.c (too small for vTaskDelay) workaround
}

void SensorsInit(void)
{
    xTaskCreatePinnedToCore(SensorsTask, "Display Task", 4096, NULL, 5, NULL, tskNO_AFFINITY);
}

static void initDevices(void)
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
    else
    {
        ESP_LOGI(TAG, "SHT21: [%x%x%x%x%x%x%x%x]", sn[0], sn[1], sn[2], 
                    sn[3], sn[4], sn[5], sn[6], sn[7]);
    }
    //=================
    gpio_config_t gpioCfg = {
        // disable interrupt
        .intr_type = GPIO_INTR_DISABLE,
        // set as output mode
        .mode = GPIO_MODE_OUTPUT,
        // bit mask of the pins that you want to set,e.g.GPIO18/19
        .pin_bit_mask = 1UL << GPIO_NUM_17,
        // disable pull-down mode
        .pull_down_en = 0,
        // disable pull-up mode
        .pull_up_en = 0
    };
    gpio_config(&gpioCfg);
    gpio_set_level(GPIO_NUM_17, 0);
    vTaskDelay(pdMS_TO_TICKS(15));
    gpio_set_level(GPIO_NUM_17, 1);
    vTaskDelay(pdMS_TO_TICKS(15));
    //=================
    bmp085.bus_write = i2cBusRegisterWrite;
    bmp085.bus_read = i2cbusRegisterRead;
    bmp085.delay_msec = delayMs;
    res = (esp_err_t) bmp085_init(&bmp085);
    if (res != ESP_OK)
    {
        ESP_LOGE(TAG, "BMP085 error: %s", esp_err_to_name(res));
    }
    //bmp085.oversampling_setting = 3;
}

static void SensorsTask(void *arg)
{
    float rh;
    float t;

    unsigned short tRaw;
    unsigned long pressRaw;
    unsigned short t2;
    unsigned long press;
    initDevices();

    while(1)
    {
        rh = Sht21GetHumidity();
        t = Sht21GetTemperature();
        ESP_LOGI(TAG, "SHT - T: %+2.1f °С H: %3.0f%%", t, rh);
        pressRaw = bmp085_get_up();
        tRaw = bmp085_get_ut();
        t2 = bmp085_get_temperature(tRaw);
        press = bmp085_get_pressure(pressRaw);
        ESP_LOGI(TAG, "BMP - T: %+2.1f °С P: %3.2f hPa", t2/10.0f, press/100.0f);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}