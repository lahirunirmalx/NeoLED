/**
 * NeoLED example: multiple strips as chained segments
 *
 * The driver transmits ONE data line from ONE I2S channel, so "multiple
 * strips" is done by wiring the strips in series and treating the whole
 * chain as a single logical array:
 *
 *   GPIO --> [strip A: 8 LEDs] DOUT --> DIN [strip B: 8 LEDs] DOUT --> ...
 *
 * Each strip is just an index range into the same frame buffer. This is the
 * supported, working pattern.
 *
 * (Truly parallel, independent strips on separate I2S peripherals are NOT
 *  supported by the current single-channel/global-buffer design — that would
 *  require multiple driver instances. See the README "Known Issues".)
 *
 * Build: define LED_NUMBER to the TOTAL number of LEDs across all segments
 * BEFORE including the header.
 */

#define LED_NUMBER 24   // 3 segments of 8 LEDs chained together
#include "neoled.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include <cstdio>

// Logical segments mapped onto the single physical chain.
struct Segment { uint16_t start; uint16_t count; };
static const Segment SEG_A = {0, 8};
static const Segment SEG_B = {8, 8};
static const Segment SEG_C = {16, 8};

static NeoLED::Pixel frame[LED_NUMBER];

static void fillSegment(const Segment& s, NeoLED::Pixel color)
{
    for (uint16_t i = 0; i < s.count; i++) {
        frame[s.start + i] = color;
    }
}

extern "C" void app_main(void)
{
    if (NeoLED::init() != NeoLED::NEOLED_OK) {
        printf("NeoLED init failed\n");
        return;
    }

    NeoLED::setBrightness(64);  // keep current draw modest while testing

    while (true) {
        // Paint each segment a different solid color.
        memset(frame, 0, sizeof(frame));
        fillSegment(SEG_A, NeoLED::COLOR_RED);
        fillSegment(SEG_B, NeoLED::COLOR_GREEN);
        fillSegment(SEG_C, NeoLED::COLOR_BLUE);
        NeoLED::update(frame);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // Rotate the colors between segments.
        fillSegment(SEG_A, NeoLED::COLOR_BLUE);
        fillSegment(SEG_B, NeoLED::COLOR_RED);
        fillSegment(SEG_C, NeoLED::COLOR_GREEN);
        NeoLED::update(frame);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
