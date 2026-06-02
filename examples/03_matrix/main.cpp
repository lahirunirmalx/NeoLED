/**
 * NeoLED example: 2D matrix (8x8)
 *
 * A NeoPixel matrix is just width*height LEDs on one chain. The only extra
 * piece is mapping (x, y) coordinates to the linear LED index. Most panels are
 * wired "serpentine" (rows alternate direction), which is what xy() handles
 * below — flip SERPENTINE to false for progressive (same-direction) panels.
 *
 * Build: LED_NUMBER must equal MATRIX_W * MATRIX_H.
 *
 * Note: large matrices produce a large DMA buffer (LED_NUMBER * 12 bytes). For
 * panels bigger than ~16x16 you may need to tune the driver's dma settings.
 */

#define MATRIX_W 8
#define MATRIX_H 8
#define LED_NUMBER (MATRIX_W * MATRIX_H)   // 64
#include "neoled.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include <cstdio>

static const bool SERPENTINE = true;

static NeoLED::Pixel frame[LED_NUMBER];

// Map (x, y) -> linear index for a serpentine-wired panel.
static inline uint16_t xy(uint8_t x, uint8_t y)
{
    if (SERPENTINE && (y & 0x01)) {
        x = (MATRIX_W - 1) - x;   // odd rows run right-to-left
    }
    return (uint16_t)(y * MATRIX_W + x);
}

extern "C" void app_main(void)
{
    if (NeoLED::init() != NeoLED::NEOLED_OK) {
        printf("NeoLED init failed\n");
        return;
    }

    NeoLED::setBrightness(48);  // matrices draw a lot of current at full white

    uint8_t hueOffset = 0;
    while (true) {
        // Diagonal rainbow that scrolls over time.
        for (uint8_t y = 0; y < MATRIX_H; y++) {
            for (uint8_t x = 0; x < MATRIX_W; x++) {
                uint8_t hue = (uint8_t)((x + y) * 16 + hueOffset);
                frame[xy(x, y)] = NeoLED::fromHSV(hue, 255, 255);
            }
        }
        NeoLED::update(frame);
        hueOffset += 4;
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}
