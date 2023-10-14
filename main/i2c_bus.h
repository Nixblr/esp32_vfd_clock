#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "stdint.h"
#include "esp_err.h"

esp_err_t i2cBusInit(void);
esp_err_t i2cbusRegisterRead(uint8_t devAddr, uint8_t regAddr, uint8_t *data, size_t len);
esp_err_t i2cBusRegisterWriteByte(uint8_t devAddr, uint8_t regAddr, uint8_t data);
esp_err_t i2cBusWriteByte(uint8_t devAddr, uint8_t data);
esp_err_t i2cBusRead(uint8_t devAddr, uint8_t *pBuf, size_t len);
esp_err_t i2cBusWriteRead(uint8_t devAddr, const uint8_t* writeBuffer, size_t writeSize,
                            uint8_t* readBuffer, size_t readSize);


#endif