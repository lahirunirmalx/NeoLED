/** MIT licence — see LICENSE.
 *
 * Host-side unit tests for the pure color utilities in neoled.h.
 *
 * These compile and run on a normal PC (no ESP-IDF / hardware) because the
 * header guards its ESP-only include behind ESP_PLATFORM. Build & run:
 *
 *     c++ -std=c++11 -I../include test_color.cpp -o test_color && ./test_color
 *
 * or simply `make` in this directory.
 */

#include <cstdio>
#include <cstdlib>
#include "neoled.h"

using namespace NeoLED;

static int g_failures = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);    \
            ++g_failures;                                                  \
        }                                                                  \
    } while (0)

static void test_makePixel()
{
    Pixel p = makePixel(10, 20, 30);
    CHECK(p.red == 10);
    CHECK(p.green == 20);
    CHECK(p.blue == 30);
}

static void test_hex_roundtrip()
{
    CHECK(hexValue(makePixel(0x12, 0x34, 0x56)) == 0x123456u);
    Pixel p = fromHex(0x123456);
    CHECK(p.red == 0x12 && p.green == 0x34 && p.blue == 0x56);
    // Deprecated alias must still work.
    Pixel q = RGBValue(0xAABBCC);
    CHECK(q.red == 0xAA && q.green == 0xBB && q.blue == 0xCC);
}

static void test_blend_endpoints()
{
    Pixel a = makePixel(10, 100, 200);
    Pixel b = makePixel(250, 50, 0);
    Pixel lo = blend(a, b, 0);
    Pixel hi = blend(a, b, 255);
    CHECK(lo.red == a.red && lo.green == a.green && lo.blue == a.blue);
    CHECK(hi.red == b.red && hi.green == b.green && hi.blue == b.blue);
}

static void test_hsv()
{
    // Saturation 0 -> grey.
    Pixel grey = fromHSV(123, 0, 200);
    CHECK(grey.red == 200 && grey.green == 200 && grey.blue == 200);
    // Hue 0, full saturation/value -> dominant red.
    Pixel red = fromHSV(0, 255, 255);
    CHECK(red.red == 255);
    CHECK(red.blue == 0);
    CHECK(red.green < 10);
}

static void test_hue_value()
{
    CHECK(hueValue(makePixel(100, 100, 100)) == 0);  // achromatic
    CHECK(hueValue(makePixel(255, 0, 0)) == 0);       // pure red -> hue 0
}

static void test_color_wheel_continuity()
{
    // Every wheel entry should have at least one non-zero channel and stay
    // within byte range (it returns uint8_t, so just exercise the full range).
    for (int h = 0; h < 256; ++h) {
        Pixel p = colorWheel((uint8_t)h);
        CHECK((p.red | p.green | p.blue) != 0);
    }
}

int main()
{
    test_makePixel();
    test_hex_roundtrip();
    test_blend_endpoints();
    test_hsv();
    test_hue_value();
    test_color_wheel_continuity();

    if (g_failures == 0) {
        std::printf("All NeoLED color tests passed.\n");
        return 0;
    }
    std::printf("%d test(s) failed.\n", g_failures);
    return 1;
}
