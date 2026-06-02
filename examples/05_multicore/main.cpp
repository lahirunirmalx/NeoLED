/**
 * NeoLED example: one strip per CPU core (dual-core)
 *
 * Each NeoLED::Strip owns its own I2S channel, buffer, and mutex, so two
 * strips can be driven from two FreeRTOS tasks pinned to the two cores with no
 * shared state between them. This keeps each strip's render+transmit loop on a
 * dedicated core — useful when one strip runs a heavy animation.
 *
 * Wiring:
 *   GPIO 21 -> strip A DIN   (I2S port 0, driven from core 0)
 *   GPIO 22 -> strip B DIN   (I2S port 1, driven from core 1)
 *
 * Note: the two Strip instances are independent. Do NOT share one Strip across
 * tasks without serialising; the per-strip mutex protects a single instance,
 * not coordination of one instance from two places.
 */

#include "neoled.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdio>

#define LEDS_PER_STRIP 16

static NeoLED::Strip stripA;
static NeoLED::Strip stripB;

// Each task owns exactly one strip and renders it forever on its core.
static void renderTask(void* arg)
{
    NeoLED::Strip* strip = static_cast<NeoLED::Strip*>(arg);
    NeoLED::Pixel frame[LEDS_PER_STRIP];

    uint8_t hue = 0;
    while (true) {
        for (int i = 0; i < LEDS_PER_STRIP; i++) {
            frame[i] = NeoLED::colorWheel((uint8_t)(hue + i * 12));
        }
        strip->update(frame);
        hue += 3;
        vTaskDelay(pdMS_TO_TICKS(25));
    }
}

extern "C" void app_main(void)
{
    if (stripA.begin(/*gpio=*/21, LEDS_PER_STRIP, /*i2s_port=*/0) != NeoLED::NEOLED_OK ||
        stripB.begin(/*gpio=*/22, LEDS_PER_STRIP, /*i2s_port=*/1) != NeoLED::NEOLED_OK) {
        printf("Strip init failed\n");
        return;
    }

    stripA.setBrightness(64);
    stripB.setBrightness(64);

    // Pin one render task to each core.
    xTaskCreatePinnedToCore(renderTask, "ledA", 4096, &stripA, 5, nullptr, 0);
    xTaskCreatePinnedToCore(renderTask, "ledB", 4096, &stripB, 5, nullptr, 1);
}
