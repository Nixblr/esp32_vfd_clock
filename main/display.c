#include "display.h"
#include "vfd.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include <string.h>
#include "esp_log.h"
#include "esp_sntp.h"
#include "lwip/apps/sntp.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_err.h"


int32_t tzAddition = 12;
char tzText[32];

static const char *TAG = "Display";
#define DISPLAY_MAX_LEN 64
#define NAMESPACE "displayStorage"
#define TZ_KEY "timezone"

static nvs_handle_t nvsStorage;

QueueHandle_t displayMessageQueue;
char displayStr[DISPLAY_MAX_LEN];

static void DisplayProcess(void *arg);

void DisplayInit(void)
{
    esp_err_t err;
    vfd_init();
    displayMessageQueue = xQueueCreate(5, sizeof(DisplayMessage_t));
    xTaskCreatePinnedToCore(DisplayProcess, "Display Task", 4096, NULL, 5, NULL, tskNO_AFFINITY);
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
    err = nvs_open(NAMESPACE, NVS_READWRITE, &nvsStorage);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    err = nvs_get_i32(nvsStorage, TZ_KEY, &tzAddition);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Can't load timezone: %s\n", esp_err_to_name(err));
    }
}

static void DisplayProcess(void *arg)
{
    DisplayMessage_t message;
    uint32_t ret;
    static char dashSign = 0;
    while (1)
    {
        ret = xQueueReceive(displayMessageQueue, &message, pdMS_TO_TICKS(500));
        if (ret == pdFALSE)
        {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            time_t sec = tv.tv_sec + ((-12 + tzAddition) * 3600);
            struct tm* tm_info = localtime(&sec);
            char stime[64];
            if (dashSign++)
            {
                strftime(stime, 64, "%H-%M-%S", tm_info);
            }
            else
            {
                strftime(stime, 64, "%H %M %S", tm_info);
            }
            dashSign &= 1;
            vfd_set_data(stime);
            continue;
        }
        strcpy(displayStr, message.msg);
        free(message.msg);
        ESP_LOGI(TAG, "New message got: %s", displayStr);
        vfd_set_data(displayStr);
        vTaskDelay(pdMS_TO_TICKS(message.showTimeMs));
    }
}

int DisplayShowMessage(const char *text, displayShowEffect_t effect, uint32_t showTime)
{
    DisplayMessage_t msg;
    char *textBuf;
    size_t buflen = strlen(text) + 1;
    textBuf = malloc(buflen);
    if (textBuf == NULL)
    {
        return -1;
    }
    memcpy(textBuf, text, buflen);
    msg.msg = textBuf;
    msg.showTimeMs = showTime;
    msg.effect = effect;
    xQueueSend(displayMessageQueue, &msg, 0);
    return 0;
}

void DisplaySetTimezone(int tz)
{
    esp_err_t err;
    tzAddition = (int32_t) tz;
    err = nvs_set_i32(nvsStorage, TZ_KEY, tzAddition);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "TZ to NVS Error: %s", esp_err_to_name(err));
    }
}

int DisplayGetTimezone(void)
{
    return tzAddition;
}