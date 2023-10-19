#ifndef SENSORS_H
#define SENSORS_H

#include "stdint.h"
#include "stdbool.h"

typedef struct BMPData_s
{
    uint32_t pressure;
    uint16_t temperature;
} BMPData_s;

typedef struct SHTData_s
{
    float rh;
    float temperature;
} SHTData_s;

typedef struct SensorsData_s
{
    SHTData_s sht21;
    BMPData_s bmp085;
} SensorsData_t;

void SensorsInit(void);
bool SensorsGetMeasure(SensorsData_t *data, uint32_t waitTime);

#endif