/**
 * NeoLED example: using the C API from a plain-C ESP-IDF project
 *
 * This file is C (not C++). Include "neoled_c.h" and drive a strip through the
 * opaque handle API — no C++ needed in your project.
 *
 * Wiring: GPIO 21 -> strip DIN (I2S port 0).
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "neoled_c.h"

#define LED_COUNT 8

void app_main(void)
{
    printf("NeoLED version %s\n", neoled_version());

    neoled_strip_handle_t strip = neoled_strip_create();
    if (!strip) {
        printf("Out of memory\n");
        return;
    }

    // GRB RGB strip on GPIO 21, I2S port 0. Use neoled_strip_begin_ex() to pick
    // a different color order (e.g. NEOLED_ORDER_RGB for WS2811) or RGBW.
    if (neoled_strip_begin(strip, 21, LED_COUNT, 0) != 0) {
        printf("Strip init failed\n");
        neoled_strip_destroy(strip);
        return;
    }

    neoled_strip_set_brightness(strip, 64);

    uint8_t hue = 0;
    while (1) {
        for (uint16_t i = 0; i < LED_COUNT; i++) {
            // Simple moving red/green/blue ramp.
            uint8_t phase = (uint8_t)(hue + i * 32);
            neoled_strip_set_pixel(strip, i, phase, 255 - phase, (uint8_t)(i * 16));
        }
        neoled_strip_show(strip);
        hue += 4;
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}
