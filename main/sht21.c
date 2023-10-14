#include "sht21.h"
#include "i2c_bus.h"
#include "stdbool.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SHT21";

#define SHT21_I2C_ADD 0x40	// I2C device address

const uint16_t POLYNOMIAL = 0x131;  // P(x)=x^8+x^5+x^4+1 = 100110001

typedef enum measureType_e {
	MT_TEMPERATURE,
	MT_RH
} measureType_t;

//==============================================================================
typedef enum SHT2xCommand_e {
  TRIG_T_MEASUREMENT_HM    = 0xE3, // command trig. temp meas. hold master
  TRIG_RH_MEASUREMENT_HM   = 0xE5, // command trig. humidity meas. hold master
  TRIG_T_MEASUREMENT_POLL  = 0xF3, // command trig. temp meas. no hold master
  TRIG_RH_MEASUREMENT_POLL = 0xF5, // command trig. humidity meas. no hold master
  USER_REG_W               = 0xE6, // command writing user register
  USER_REG_R               = 0xE7, // command reading user register
  SOFT_RESET               = 0xFE  // command soft reset
} SHT2xCommand_t;
//==============================================================================

static float CalcRH(uint16_t rh);
static float CalcT(uint16_t t);
static uint8_t CRCCheck(uint8_t data[], uint8_t no_of_bytes, uint8_t checksum);
static bool ReadSensorHM(measureType_t mt, uint16_t *pResult);


float Sht21GetHumidity(void) {
	uint16_t result; 	// return variable
	if (!ReadSensorHM(MT_RH, &result))
	{
		return 0.0f;
	}
	return CalcRH(result);
}

float Sht21GetTemperature(void) {
	uint16_t result; 	// return variable
	if (!ReadSensorHM(MT_TEMPERATURE, &result))
	{
		return 0.0f;
	}
	return CalcT(result);
}

void Sht21Reset() {
	i2cBusWriteByte(SHT21_I2C_ADD, SOFT_RESET);
}

esp_err_t Sht21GetSerialNumber(uint8_t *return_sn) {
    esp_err_t res;
	uint8_t block1[8];
	uint8_t block2[6];
	const uint8_t cmd1[] = {0xfa, 0x0f};
	const uint8_t cmd2[] = {0xfc, 0xc9};
	res = i2cBusWriteRead(SHT21_I2C_ADD, cmd1, 2, block1, 8);
    if (res != ESP_OK)
    {
        return res;
    }
	res = i2cBusWriteRead(SHT21_I2C_ADD, cmd2, 2, block2, 6);
    if (res != ESP_OK)
    {
        return res;
    }
    return_sn[5] = block1[0];
    return_sn[4] = block1[2];
    return_sn[3] = block1[4];
    return_sn[2] = block1[6];
    return_sn[1] = block2[0];
    return_sn[0] = block2[1];
    return_sn[7] = block2[3];
    return_sn[6] = block2[4];
	return ESP_OK;
}

//==============================================================================

static bool ReadSensorHM(measureType_t mt, uint16_t *pResult) {
	uint8_t command;
	uint8_t rxData[3];
	uint8_t n = 0;
	uint8_t d;
	if (mt == MT_TEMPERATURE)
	{
		command = TRIG_T_MEASUREMENT_POLL;
	}
	else if (mt == MT_RH)
	{
		command = TRIG_RH_MEASUREMENT_POLL;
	}
	else
	{
		ESP_LOGE(TAG, "Measure: invalid argument");
		return false;
	}
	//esp_err_t res = i2cBusWriteRead(SHT21_I2C_ADD, &command, 1, rxData, 3);
	esp_err_t res = i2cBusWriteByte(SHT21_I2C_ADD, command);
	if (res != ESP_OK)
	{
		ESP_LOGE(TAG, "1 Measure IO error: %s", esp_err_to_name(res));
		return false;
	}
	vTaskDelay(pdMS_TO_TICKS(100));
	
	res = i2cBusRead(SHT21_I2C_ADD, rxData, 3);
	if (res != ESP_OK)
	{
		ESP_LOGE(TAG, "2 Measure IO error: %s", esp_err_to_name(res));
		return false;
	}
	*pResult = (rxData[0] << 8);
	*pResult += rxData[1];
	if(CRCCheck (rxData,2,rxData[2])) 
	{
		ESP_LOGE(TAG, "Read measure CHKSUM error");
		return false;
	}
	return true;
}

static float CalcRH(uint16_t rh) {

	rh &= ~0x0003;	// clean last two bits

  	return (-6.0 + 125.0/65536 * (float)rh); // return relative humidity
}

static float CalcT(uint16_t t) {

	t &= ~0x0003;	// clean last two bits
 	
	return (-46.85 + 175.72/65536 * (float)t);
}

static uint8_t CRCCheck(uint8_t data[], uint8_t no_of_bytes, uint8_t checksum) {
	uint8_t crc = 0;	
  	uint8_t byteCtr;

 	 //calculates 8-Bit checksum with given polynomial
  	for (byteCtr = 0; byteCtr < no_of_bytes; ++byteCtr)
 	 { crc ^= (data[byteCtr]);
 	   for (uint8_t bit = 8; bit > 0; --bit)
 	   { if (crc & 0x80) crc = (crc << 1) ^ POLYNOMIAL;
 	     else crc = (crc << 1);
 	   }
 	 }
 	 if (crc != checksum) return 1;
 	 else return 0;
}