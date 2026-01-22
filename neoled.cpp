/** MIT licence

 Copyright (C) 2019 by Vu Nam https://github.com/vunam https://studiokoda.com
 Copyright (C) 2024-2026 Contributors

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
#include "esp_log.h"
#include "driver/gpio.h"
#include "neoled.h"

// ESP-IDF version-specific includes
#if NEOLED_USE_NEW_I2S_DRIVER
    // ESP-IDF 5.x uses new I2S driver
    #include "driver/i2s_std.h"
#else
    // ESP-IDF 4.x uses legacy I2S driver
    #include "driver/i2s.h"
#endif

static const char* TAG = "NeoLED";

namespace NeoLED {

// ============================================================================
// Static Variables
// ============================================================================

static uint8_t out_buffer[LED_NUMBER * PIXEL_SIZE] = {0};
static uint8_t off_buffer[ZERO_BUFFER] = {0};
static uint16_t size_buffer = 0;
static bool initialized = false;
static uint8_t global_brightness = 255;
static int current_gpio_pin = I2S_DO_IO;

// Bit patterns for WS2812 timing via I2S
static const uint16_t bitpatterns[4] = {0x88, 0x8e, 0xe8, 0xee};

// ============================================================================
// I2S Configuration (Version-specific)
// ============================================================================

#if NEOLED_USE_NEW_I2S_DRIVER
    // ESP-IDF 5.x: New I2S driver handle
    static i2s_chan_handle_t tx_handle = NULL;
#else
    // ESP-IDF 4.x: Legacy I2S configuration
    static i2s_config_t i2s_config = {
        .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
        .communication_format = static_cast<i2s_comm_format_t>(I2S_COMM_FORMAT_STAND_I2S | I2S_COMM_FORMAT_STAND_MSB),
    #else
        .communication_format = static_cast<i2s_comm_format_t>(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    #endif
        .intr_alloc_flags = 0,
        .dma_buf_count = 4,
        .dma_buf_len = LED_NUMBER * PIXEL_SIZE,
        .use_apll = false,
    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
        .mclk_multiple = I2S_MCLK_MULTIPLE_DEFAULT,
    #endif
        .tx_desc_auto_clear = true
    };

    static i2s_pin_config_t pin_config = {
    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
        .mck_io_num = I2S_PIN_NO_CHANGE,
    #endif
        .bck_io_num = I2S_PIN_NO_CHANGE,
        .ws_io_num = I2S_PIN_NO_CHANGE,
        .data_out_num = I2S_DO_IO,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
#endif

// ============================================================================
// Internal Helper Functions
// ============================================================================

/**
 * @brief Convert pixel data to I2S bit patterns
 * @param pixel Source pixel
 * @param buffer Output buffer (must be at least PIXEL_SIZE bytes)
 * @param brightness Brightness multiplier (0-255)
 */
static void pixelToBitPattern(const Pixel& pixel, uint8_t* buffer, uint8_t brightness)
{
    // Apply brightness
    uint8_t r = (uint8_t)((pixel.red * brightness) / 255);
    uint8_t g = (uint8_t)((pixel.green * brightness) / 255);
    uint8_t b = (uint8_t)((pixel.blue * brightness) / 255);

    // Green first (WS2812 uses GRB format)
    buffer[0] = bitpatterns[g >> 6 & 0x03];
    buffer[1] = bitpatterns[g >> 4 & 0x03];
    buffer[2] = bitpatterns[g >> 2 & 0x03];
    buffer[3] = bitpatterns[g & 0x03];

    // Red
    buffer[4] = bitpatterns[r >> 6 & 0x03];
    buffer[5] = bitpatterns[r >> 4 & 0x03];
    buffer[6] = bitpatterns[r >> 2 & 0x03];
    buffer[7] = bitpatterns[r & 0x03];

    // Blue
    buffer[8] = bitpatterns[b >> 6 & 0x03];
    buffer[9] = bitpatterns[b >> 4 & 0x03];
    buffer[10] = bitpatterns[b >> 2 & 0x03];
    buffer[11] = bitpatterns[b & 0x03];
}

// ============================================================================
// Core Functions Implementation
// ============================================================================

neoled_err_t init(void)
{
    return initWithPin(I2S_DO_IO);
}

neoled_err_t initWithPin(int gpio_pin)
{
    if (initialized) {
        ESP_LOGW(TAG, "Already initialized, call destroy() first");
        return NEOLED_OK;  // Already initialized is not an error
    }

    size_buffer = LED_NUMBER * PIXEL_SIZE;
    current_gpio_pin = gpio_pin;
    esp_err_t ret;

#if NEOLED_USE_NEW_I2S_DRIVER
    // ESP-IDF 5.x: New I2S driver initialization
    i2s_chan_config_t chan_cfg = {
        .id = (i2s_port_t)I2S_NUM,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 4,
        .dma_frame_num = LED_NUMBER * PIXEL_SIZE,
        .auto_clear = true
    };

    ret = i2s_new_channel(&chan_cfg, &tx_handle, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2S channel: %s", esp_err_to_name(ret));
        return NEOLED_ERR_I2S;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_DEFAULT
        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode = I2S_SLOT_MODE_STEREO,
            .slot_mask = I2S_STD_SLOT_BOTH,
            .ws_width = I2S_DATA_BIT_WIDTH_16BIT,
            .ws_pol = false,
            .bit_shift = true,
            .left_align = false,
            .big_endian = false,
            .bit_order_lsb = false
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_GPIO_UNUSED,
            .ws = I2S_GPIO_UNUSED,
            .dout = (gpio_num_t)gpio_pin,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false
            }
        }
    };

    ret = i2s_channel_init_std_mode(tx_handle, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init I2S channel: %s", esp_err_to_name(ret));
        i2s_del_channel(tx_handle);
        tx_handle = NULL;
        return NEOLED_ERR_I2S;
    }

    ret = i2s_channel_enable(tx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S channel: %s", esp_err_to_name(ret));
        i2s_del_channel(tx_handle);
        tx_handle = NULL;
        return NEOLED_ERR_I2S;
    }

#else
    // ESP-IDF 4.x: Legacy I2S driver initialization
    pin_config.data_out_num = gpio_pin;

    ret = i2s_driver_install(static_cast<i2s_port_t>(I2S_NUM), &i2s_config, 0, nullptr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2S driver: %s", esp_err_to_name(ret));
        return NEOLED_ERR_I2S;
    }

    ret = i2s_set_pin(static_cast<i2s_port_t>(I2S_NUM), &pin_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set I2S pins: %s", esp_err_to_name(ret));
        i2s_driver_uninstall(static_cast<i2s_port_t>(I2S_NUM));
        return NEOLED_ERR_I2S;
    }
#endif

    initialized = true;
    ESP_LOGI(TAG, "Initialized with GPIO %d, %d LEDs", gpio_pin, LED_NUMBER);
    
    // Clear LEDs on init
    clear();
    
    return NEOLED_OK;
}

neoled_err_t update(const Pixel* pixels)
{
    return updateWithBrightness(pixels, global_brightness);
}

neoled_err_t updateWithBrightness(const Pixel* pixels, uint8_t brightness)
{
    if (!initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return NEOLED_ERR_NOT_INIT;
    }

    if (pixels == nullptr) {
        ESP_LOGE(TAG, "Null pixel pointer");
        return NEOLED_ERR_PARAM;
    }

    // Convert all pixels to bit patterns
    for (uint16_t i = 0; i < LED_NUMBER; i++) {
        int loc = i * PIXEL_SIZE;
        pixelToBitPattern(pixels[i], &out_buffer[loc], brightness);
    }

    size_t bytes_written = 0;
    esp_err_t ret;

#if NEOLED_USE_NEW_I2S_DRIVER
    // ESP-IDF 5.x: New I2S write
    ret = i2s_channel_write(tx_handle, out_buffer, size_buffer, &bytes_written, portMAX_DELAY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S write failed: %s", esp_err_to_name(ret));
        return NEOLED_ERR_I2S;
    }

    ret = i2s_channel_write(tx_handle, off_buffer, ZERO_BUFFER, &bytes_written, portMAX_DELAY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S write (reset) failed: %s", esp_err_to_name(ret));
        return NEOLED_ERR_I2S;
    }
#else
    // ESP-IDF 4.x: Legacy I2S write
    ret = i2s_write(static_cast<i2s_port_t>(I2S_NUM), out_buffer, size_buffer, &bytes_written, portMAX_DELAY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S write failed: %s", esp_err_to_name(ret));
        return NEOLED_ERR_I2S;
    }

    ret = i2s_write(static_cast<i2s_port_t>(I2S_NUM), off_buffer, ZERO_BUFFER, &bytes_written, portMAX_DELAY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S write (reset) failed: %s", esp_err_to_name(ret));
        return NEOLED_ERR_I2S;
    }
#endif

    // Small delay for data latch
    vTaskDelay(pdMS_TO_TICKS(1));

#if NEOLED_USE_NEW_I2S_DRIVER
    // Clear DMA buffer in new driver if needed
#else
    i2s_zero_dma_buffer(static_cast<i2s_port_t>(I2S_NUM));
#endif

    return NEOLED_OK;
}

neoled_err_t clear(void)
{
    if (!initialized) {
        return NEOLED_ERR_NOT_INIT;
    }

    // Create array of off pixels
    Pixel off_pixels[LED_NUMBER];
    memset(off_pixels, 0, sizeof(off_pixels));
    
    return updateWithBrightness(off_pixels, 0);
}

neoled_err_t destroy(void)
{
    if (!initialized) {
        return NEOLED_OK;  // Not an error to destroy when not initialized
    }

    // Turn off LEDs before destroying
    clear();

    esp_err_t ret;

#if NEOLED_USE_NEW_I2S_DRIVER
    // ESP-IDF 5.x: Cleanup new I2S driver
    if (tx_handle != NULL) {
        ret = i2s_channel_disable(tx_handle);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to disable I2S channel: %s", esp_err_to_name(ret));
        }

        ret = i2s_del_channel(tx_handle);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to delete I2S channel: %s", esp_err_to_name(ret));
        }
        tx_handle = NULL;
    }
#else
    // ESP-IDF 4.x: Cleanup legacy I2S driver
    ret = i2s_driver_uninstall(static_cast<i2s_port_t>(I2S_NUM));
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to uninstall I2S driver: %s", esp_err_to_name(ret));
    }
#endif

    // Reset GPIO pin
    gpio_reset_pin(static_cast<gpio_num_t>(current_gpio_pin));

    initialized = false;
    ESP_LOGI(TAG, "Destroyed");

    return NEOLED_OK;
}

bool isInitialized(void)
{
    return initialized;
}

void setBrightness(uint8_t brightness)
{
    global_brightness = brightness;
}

uint8_t getBrightness(void)
{
    return global_brightness;
}

// ============================================================================
// Gamma Correction
// ============================================================================

// Gamma correction lookup table (gamma = 2.2)
static const uint8_t gamma22_table[256] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,
    2,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   5,   5,   5,
    5,   6,   6,   6,   6,   7,   7,   7,   7,   8,   8,   8,   9,   9,   9,  10,
   10,  10,  11,  11,  11,  12,  12,  13,  13,  13,  14,  14,  15,  15,  16,  16,
   17,  17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  22,  23,  24,  24,  25,
   25,  26,  27,  27,  28,  29,  29,  30,  31,  32,  32,  33,  34,  35,  35,  36,
   37,  38,  39,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  50,
   51,  52,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  66,  67,  68,
   69,  70,  72,  73,  74,  75,  77,  78,  79,  81,  82,  83,  85,  86,  87,  89,
   90,  92,  93,  95,  96,  98,  99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};

Pixel gammaCorrect(const Pixel& pixel, float gamma)
{
    Pixel result;
    
    if (gamma == 2.2f) {
        // Use lookup table for common gamma value
        result.red = gamma22_table[pixel.red];
        result.green = gamma22_table[pixel.green];
        result.blue = gamma22_table[pixel.blue];
    } else {
        // Calculate gamma for custom values
        result.red = (uint8_t)(powf(pixel.red / 255.0f, gamma) * 255.0f + 0.5f);
        result.green = (uint8_t)(powf(pixel.green / 255.0f, gamma) * 255.0f + 0.5f);
        result.blue = (uint8_t)(powf(pixel.blue / 255.0f, gamma) * 255.0f + 0.5f);
    }
    
    return result;
}

} // namespace NeoLED
