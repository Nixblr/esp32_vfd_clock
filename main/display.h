#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

typedef enum displayShowEffect_e
{
    DSE_NONE,
    DSE_SCROLL_LEFT,
    DSE_SCROLL_RIGHT
} displayShowEffect_t;

typedef struct DisplayMessage_s
{
    char *msg;
    uint32_t showTimeMs;
    displayShowEffect_t effect;
} DisplayMessage_t;

void DisplayInit(void);
int DisplayShowMessage(const char *text, displayShowEffect_t effect, uint32_t showTime);
void DisplaySetTimezone(int tz);
int DisplayGetTimezone(void);

#endif