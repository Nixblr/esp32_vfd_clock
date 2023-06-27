#include "display.h"
#include "vfd.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include <string.h>
#include "esp_log.h"

static const char *TAG = "Display";
#define DISPLAY_MAX_LEN 64

QueueHandle_t displayMessageQueue;
char displayStr[DISPLAY_MAX_LEN];

static void DisplayProcess(void *arg);

void DisplayInit(void)
{
    vfd_init();
    displayMessageQueue = xQueueCreate(5, sizeof(DisplayMessage_t));
    xTaskCreatePinnedToCore(DisplayProcess, "Display Task", 4096, NULL, 5, NULL, tskNO_AFFINITY);
}

static void DisplayProcess(void *arg)
{
    DisplayMessage_t message;
    uint32_t ret;
    while (1)
    {
        ret = xQueueReceive(displayMessageQueue, &message, portMAX_DELAY);
        if (ret == pdFALSE)
        {
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