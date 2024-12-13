#ifndef SHT21_H
#define SHT21_H

#include "stdint.h"
#include "esp_err.h"

void Sht21Reset();
esp_err_t Sht21GetSerialNumber(uint8_t *return_sn);
float Sht21GetHumidity(void);
float Sht21GetTemperature(void);

#endif