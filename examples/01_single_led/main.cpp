/**
 * NeoLED example: single LED
 *
 * The simplest possible use: one WS2812 on the default data pin.
 * Build with LED_NUMBER = 1 (the library default), so no override is needed.
 *
 * Wiring:
 *   ESP32 GPIO (I2S_DO_IO, default 21) --> LED DIN
 *   5V / GND shared between board and LED.
 */

#include "neoled.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdio>

extern "C" void app_main(void)
{
    if (NeoLED::init() != NeoLED::NEOLED_OK) {
        printf("NeoLED init failed\n");
        return;
    }

    // A gentle "breathing" white using per-update brightness.
    while (true) {
        for (int b = 0; b <= 255; b += 5) {
            NeoLED::Pixel white = NeoLED::COLOR_WHITE;
            NeoLED::updateWithBrightness(&white, (uint8_t)b);
            vTaskDelay(pdMS_TO_TICKS(15));
        }
        for (int b = 255; b >= 0; b -= 5) {
            NeoLED::Pixel white = NeoLED::COLOR_WHITE;
            NeoLED::updateWithBrightness(&white, (uint8_t)b);
            vTaskDelay(pdMS_TO_TICKS(15));
        }
    }
}
