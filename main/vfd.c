/*
 * vfd.c
 *
 *  Created on: 21 ���. 2021 �.
 *      Author: Nix
 *
 *      VFD latch bits:
 *
 *      0 - EMPTY
 *      1 - EMPTY
 *      2 - EMPTY
 *      3 - c
 *      4 - G10
 *      5 - b
 *      6 - a
 *      7 - EMPTY
 *
 *      8 - G4
 *      9 - G5
 *      10- G6
 *      11- G7
 *      12- G8
 *      13- d
 *      14- G9
 *      15- EMPTY
 *
 *      16- f
 *      17- G1
 *      18- g
 *      19- G2
 *      20- e
 *      21- h
 *      22- G3
 *      23- EMPTY
 *
 *      XT1 Pinout:
 *
 *      1 - CLK (GPIO14)
 *      2 - WR (GPIO25)
 *      3 - /ENABLE (---)
 *      4 - DATA (GPIO13)
 *
 *      XT2 Pinout:
 *
 *      1 - -30V
 *      2 - +5V
 *      3 - GND
 *      4 - GND
 *      5 - H1
 *      6 - H1
 */

#include "vfd.h"
#include <stdio.h>
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "string.h"

#define PIN_NUM_MOSI 13
#define PIN_NUM_CLK  14


//      Segment code macro
#define SEG_A 0x7FFFBF
#define SEG_B 0x7FFFDF
#define SEG_C 0x7FFFF7
#define SEG_D 0x7FDFFF
#define SEG_E 0x6FFFFF
#define SEG_F 0x7EFFFF
#define SEG_G 0x7BFFFF
#define SEG_H 0x5FFFFF
#define SEG_TABLE_END 0xFFFFFFFF

typedef struct {uint16_t SymCode;
                uint32_t GraphCode;
} graph_element_t;

static void vfd_proc(void *arg);
static void send_raw_data(uint32_t raw);

uint32_t graph_buffer[10];

//            Grid table

const uint32_t grid_pos_conv_table[10] = {0xfdffff, 0xf7ffff, 0xbfffff,
                                          0xfffeff, 0xfffdff, 0xfFFBFF,
                                          0xfFF7FF, 0xfFEFFF, 0xfFBFFF,
                                          0xfFFFEF};


//            Symbol table

const graph_element_t graph_conv_table[] = {
{'0', SEG_A & SEG_B & SEG_C & SEG_D & SEG_E & SEG_F},
{'1', SEG_B & SEG_C},
{'2', SEG_A & SEG_B & SEG_D & SEG_E & SEG_G},
{'3', SEG_A & SEG_B & SEG_C & SEG_D & SEG_G},
{'4', SEG_B & SEG_C & SEG_F & SEG_G},
{'5', SEG_A & SEG_C & SEG_D & SEG_F & SEG_G},
{'6', SEG_A & SEG_C & SEG_D & SEG_E & SEG_F & SEG_G},
{'7', SEG_A & SEG_B & SEG_C},
{'8', SEG_A & SEG_B & SEG_C & SEG_D & SEG_E & SEG_F & SEG_G},
{'9', SEG_A & SEG_B & SEG_C & SEG_D & SEG_F & SEG_G},
{'A', SEG_A & SEG_B & SEG_C & SEG_E & SEG_F & SEG_G},
{'b', SEG_C & SEG_D & SEG_E & SEG_F & SEG_G},
{'C', SEG_A & SEG_D & SEG_E & SEG_F},
{'d', SEG_B & SEG_C & SEG_D & SEG_E & SEG_G},
{'E', SEG_A & SEG_D & SEG_E & SEG_F & SEG_G},
{'F', SEG_A & SEG_E & SEG_F & SEG_G},
{'G', SEG_A & SEG_C & SEG_D & SEG_E & SEG_F},
{'H', SEG_B & SEG_C & SEG_E & SEG_F & SEG_G},
{'I', SEG_B & SEG_C},
{'J', SEG_B & SEG_C & SEG_D},
{'L', SEG_D & SEG_E & SEG_F},
{'n', SEG_C & SEG_E & SEG_E & SEG_G},
{'O', SEG_A & SEG_B & SEG_C & SEG_D & SEG_E & SEG_F},
{'o', SEG_C & SEG_D & SEG_E & SEG_G},
{'P', SEG_A & SEG_B & SEG_E & SEG_F & SEG_G},
{'r', SEG_E & SEG_G},
{'S', SEG_A & SEG_C & SEG_D & SEG_F & SEG_G},
{'t', SEG_D & SEG_E & SEG_F & SEG_G},
{'u', SEG_C & SEG_D & SEG_E},
{'V', SEG_B & SEG_C & SEG_D & SEG_E & SEG_F},
{'Y', SEG_B & SEG_C & SEG_D & SEG_F & SEG_G},
{'-', SEG_G},
{' ', 0xffffff},
{',', SEG_H},
{0, SEG_TABLE_END}
};

spi_device_handle_t spi2; //SPI Handle
spi_transaction_t transaction;

const esp_timer_create_args_t periodic_timer_args = {
            .callback = &vfd_proc,
            .name = "VFD periodic"
    };

esp_timer_handle_t periodic_timer;

void vfd_init(void) {
    esp_err_t ret;

    printf("\r\n***ESP32 VFD Init***\r\n");

    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };

    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    spi_device_interface_config_t devcfg={
    .clock_speed_hz = 1000000,  // 1 MHz
    .mode = 0,                  //SPI mode 0
    .spics_io_num = 25,         // CS Pin
    .queue_size = 1,
    .flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_TXBIT_LSBFIRST,
    .pre_cb = NULL,
    .post_cb = NULL,
    };

    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi2);
    ESP_ERROR_CHECK(ret);
    vfd_set_data("          ");
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000));
}

static void vfd_proc(void *arg)
{
    uint32_t raw_data;
    static uint16_t grid_number;

    // switch active grid
    if (++grid_number > 9)
    {
        grid_number = 0;
    }
    raw_data = grid_pos_conv_table[grid_number] & graph_buffer[grid_number];
    send_raw_data(raw_data);
}

void vfd_set_data(char *data) {
    uint16_t iter, searchPos;
    memset(graph_buffer, 0xff, sizeof(graph_buffer));
    for (iter = 0; iter < 9; iter++)
    {
        if (data[iter] == 0)
        {
            break;
        }
        searchPos = 0;
        while (graph_conv_table[searchPos].GraphCode != SEG_TABLE_END)
        {
            if (graph_conv_table[searchPos].SymCode == data[iter])
            {
                graph_buffer[iter + 1] = graph_conv_table[searchPos].GraphCode;
                break;
            }
            searchPos++;
        }
    }
}

static void send_raw_data(uint32_t raw) {
    static uint8_t dat[3];
    dat[0] = raw & 0xff;
    dat[1] = (raw >> 8) & 0xff;
    dat[2] = (raw >> 16) & 0xff;
    esp_err_t ret;
    transaction.tx_buffer = dat;
    transaction.length = 3 * 8;
    ret = spi_device_queue_trans(spi2, &transaction, 1);
    ESP_ERROR_CHECK(ret);
}