/**
 * NeoLED example: two independent strips on two I2S channels, in parallel
 *
 * The ESP32 (and ESP32-S3) have two I2S peripherals, so two separate strips
 * on two different GPIOs can be driven at the same time. Each strip is a
 * NeoLED::Strip instance with its own I2S port; updateParallel() fills both
 * buffers and kicks both DMA transfers back-to-back, so they refresh together.
 *
 * Wiring:
 *   GPIO 21 -> strip 0 DIN   (I2S port 0)
 *   GPIO 22 -> strip 1 DIN   (I2S port 1)
 *
 * SoCs with a single I2S peripheral (S2/C3/C6/H2) can still use the Strip
 * class for one strip, but not two in parallel.
 */

#include "neoled.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdio>

#define STRIP0_LEDS 16
#define STRIP1_LEDS 16

static NeoLED::Strip strip0;
static NeoLED::Strip strip1;

static NeoLED::Pixel frame0[STRIP0_LEDS];
static NeoLED::Pixel frame1[STRIP1_LEDS];

extern "C" void app_main(void)
{
    if (strip0.begin(/*gpio=*/21, STRIP0_LEDS, /*i2s_port=*/0) != NeoLED::NEOLED_OK ||
        strip1.begin(/*gpio=*/22, STRIP1_LEDS, /*i2s_port=*/1) != NeoLED::NEOLED_OK) {
        printf("Strip init failed (check that this SoC has 2 I2S ports)\n");
        return;
    }

    strip0.setBrightness(64);
    strip1.setBrightness(64);

    NeoLED::Strip* strips[]      = { &strip0, &strip1 };
    const NeoLED::Pixel* frames[] = { frame0, frame1 };

    uint8_t hue = 0;
    while (true) {
        // Strip 0 sweeps the color wheel; strip 1 runs the opposite phase.
        for (int i = 0; i < STRIP0_LEDS; i++) {
            frame0[i] = NeoLED::colorWheel((uint8_t)(hue + i * 8));
        }
        for (int i = 0; i < STRIP1_LEDS; i++) {
            frame1[i] = NeoLED::colorWheel((uint8_t)(128 + hue + i * 8));
        }

        // Both strips transmit effectively simultaneously.
        NeoLED::updateParallel(strips, frames, 2);

        hue += 2;
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}
