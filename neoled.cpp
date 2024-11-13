/** MIT licence

 Copyright (C) 2019 by Vu Nam https://github.com/vunam https://studiokoda.com

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do
 so, subject to the following conditions: The above copyright notice and this
 permission notice shall be included in all copies or substantial portions of
 the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 DEALINGS IN THE SOFTWARE.

*/

#include <cmath>
#include <cstdio>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/i2s.h"
#include "driver/gpio.h"   
#include "neoled.h"

namespace NeoLED {

static uint8_t out_buffer[LED_NUMBER * PIXEL_SIZE] = {0};
static uint8_t off_buffer[ZERO_BUFFER] = {0};
static uint16_t size_buffer;

static const uint16_t bitpatterns[4] = {0x88, 0x8e, 0xe8, 0xee};

i2s_config_t i2s_config = {
    .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = static_cast<i2s_comm_format_t> ( I2S_COMM_FORMAT_STAND_I2S | I2S_COMM_FORMAT_STAND_MSB),
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,
    .dma_buf_len = LED_NUMBER * PIXEL_SIZE,
    .use_apll = false,
    .mclk_multiple = I2S_MCLK_MULTIPLE_DEFAULT
};

i2s_pin_config_t pin_config = {
    .mck_io_num = static_cast<gpio_num_t>(1),
    .bck_io_num = static_cast<gpio_num_t>(-1),
    .ws_io_num = static_cast<gpio_num_t>(-1),
    .data_out_num = static_cast<gpio_num_t>(I2S_DO_IO),
    .data_in_num = static_cast<gpio_num_t>(-1)
    
};

void init() {
    size_buffer = LED_NUMBER * PIXEL_SIZE;
    i2s_driver_install(static_cast<i2s_port_t>(I2S_NUM), &i2s_config, 0, nullptr);
    i2s_set_pin(static_cast<i2s_port_t>(I2S_NUM), &pin_config);
}

void  destroy() {
    i2s_driver_uninstall(static_cast<i2s_port_t>(I2S_NUM));
    gpio_reset_pin(static_cast<gpio_num_t>(I2S_DO_IO));
}

void update(Pixel* pixels) {
    size_t bytes_written = 0;

    for (uint16_t i = 0; i < LED_NUMBER; i++) {
        int loc = i * PIXEL_SIZE;

        out_buffer[loc] = bitpatterns[pixels[i].green >> 6 & 0x03];
        out_buffer[loc + 1] = bitpatterns[pixels[i].green >> 4 & 0x03];
        out_buffer[loc + 2] = bitpatterns[pixels[i].green >> 2 & 0x03];
        out_buffer[loc + 3] = bitpatterns[pixels[i].green & 0x03];

        out_buffer[loc + 4] = bitpatterns[pixels[i].red >> 6 & 0x03];
        out_buffer[loc + 5] = bitpatterns[pixels[i].red >> 4 & 0x03];
        out_buffer[loc + 6] = bitpatterns[pixels[i].red >> 2 & 0x03];
        out_buffer[loc + 7] = bitpatterns[pixels[i].red & 0x03];

        out_buffer[loc + 8] = bitpatterns[pixels[i].blue >> 6 & 0x03];
        out_buffer[loc + 9] = bitpatterns[pixels[i].blue >> 4 & 0x03];
        out_buffer[loc + 10] = bitpatterns[pixels[i].blue >> 2 & 0x03];
        out_buffer[loc + 11] = bitpatterns[pixels[i].blue & 0x03];
    }

    i2s_write(static_cast<i2s_port_t>(I2S_NUM), out_buffer, size_buffer, &bytes_written, portMAX_DELAY);
    i2s_write(static_cast<i2s_port_t>(I2S_NUM), off_buffer, ZERO_BUFFER, &bytes_written, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(10));
    i2s_zero_dma_buffer(static_cast<i2s_port_t>(I2S_NUM));
}

} // namespace NeoLED
