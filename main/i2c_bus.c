#include "i2c_bus.h"
#include "driver/i2c.h"

//static const char *TAG = "I2C BUS";

#define I2C_MASTER_SCL_IO           CONFIG_I2C_MASTER_SCL      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           CONFIG_I2C_MASTER_SDA      /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

esp_err_t i2cBusInit(void)
{
    esp_err_t res;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_19,
        .scl_io_num = GPIO_NUM_18,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    res = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (res != ESP_OK)
    {
        return res;
    }
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

esp_err_t i2cbusRegisterRead(uint8_t devAddr, uint8_t regAddr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, devAddr, &regAddr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

esp_err_t i2cBusRead(uint8_t devAddr, uint8_t *pBuf, size_t len)
{
    return i2c_master_read_from_device(I2C_MASTER_NUM, devAddr, pBuf, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

esp_err_t i2cBusRegisterWriteByte(uint8_t devAddr, uint8_t regAddr, uint8_t data)
{
    int ret;
    uint8_t write_buf[2] = {regAddr, data};
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, devAddr, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    return ret;
}

esp_err_t i2cBusWriteByte(uint8_t devAddr, uint8_t data)
{
    int ret;
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, devAddr, &data, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    return ret;
}

esp_err_t i2cBusWriteRead(uint8_t devAddr, const uint8_t* writeBuffer, size_t writeSize,
                            uint8_t* readBuffer, size_t readSize)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, devAddr, writeBuffer, writeSize, 
                                        readBuffer, readSize, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}