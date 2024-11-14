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
#ifndef NEOLED_H
#define NEOLED_H

#include <cstdint>

namespace NeoLED {

#define LED_NUMBER 1
#define PIXEL_SIZE 12 // each color takes 4 bytes
#define SAMPLE_RATE (93750)
#define ZERO_BUFFER 48
#define I2S_NUM (0)
#define I2S_DO_IO (21)


#define COLOR_RED        (Pixel){0, 255, 0}
#define COLOR_ORANGE     (Pixel){64, 255, 0}
#define COLOR_YELLOW     (Pixel){128, 255, 0}
#define COLOR_LIME       (Pixel){255, 255, 0}
#define COLOR_GREEN      (Pixel){255, 0, 0}
#define COLOR_TURQUOISE  (Pixel){255, 0, 128}
#define COLOR_CYAN       (Pixel){255, 0, 255}
#define COLOR_AQUA       (Pixel){128, 0, 255}
#define COLOR_BLUE       (Pixel){0, 0, 255}
#define COLOR_PURPLE     (Pixel){0, 128, 255}
#define COLOR_MAGENTA    (Pixel){0, 255, 255}
#define COLOR_ROSE       (Pixel){0, 255, 128}
#define COLOR_WHITE      (Pixel){255, 255, 255}
#define COLOR_OFF        (Pixel){0, 0, 0}


#define HUE_RED        0     // Red
#define HUE_ORANGE     32    // Orange
#define HUE_YELLOW     64    // Yellow
#define HUE_LIME       80    // Lime Green
#define HUE_GREEN      96    // Green
#define HUE_TURQUOISE  112   // Turquoise
#define HUE_CYAN       128   // Cyan
#define HUE_AQUA       144   // Aqua Blue
#define HUE_BLUE       160   // Blue
#define HUE_PURPLE     176   // Purple
#define HUE_MAGENTA    192   // Magenta (Pink)
#define HUE_ROSE       224   // Rose Pink
#define HUE_WHITE      0     // Use full brightness for white (no hue shift)
#define HUE_OFF        0     // Turn off LED (set RGB values to zero)

    typedef struct
    {
        uint8_t green;
        uint8_t red;
        uint8_t blue;
    } Pixel;

    void init();
    void update(Pixel* pixels);
    void destroy();

    inline Pixel makePixel(uint8_t r, uint8_t g, uint8_t b)
    {
        Pixel pixel;
        pixel.red = r;
        pixel.green = g;
        pixel.blue = b;
        return pixel;
    }
    inline Pixel colorWheel(uint8_t hue)
    {
        Pixel pixel;

        if (hue < 85)
        {
            pixel.red = hue * 3;
            pixel.green = 255 - hue * 3;
            pixel.blue = 0;
        }
        else if (hue < 170)
        {
            hue -= 85;
            pixel.red = 255 - hue * 3;
            pixel.green = 0;
            pixel.blue = hue * 3;
        }
        else
        {
            hue -= 170;
            pixel.red = 0;
            pixel.green = hue * 3;
            pixel.blue = 255 - hue * 3;
        }

        return pixel;
    }
    inline uint8_t hueValue(Pixel pixel)
    {
        if (pixel.red > 0 && pixel.blue == 0) { 
        return pixel.red / 3;
    }
    if (pixel.red > 0 && pixel.green == 0) { 
        return 85 + (255 - pixel.red) / 3;
    }
    if (pixel.green > 0 && pixel.red == 0) { 
        return 170 + pixel.green / 3;
    } 
    return 0;
    }
     inline uint32_t hexValue(Pixel pixel)
    {
        return ((uint32_t)pixel.red << 16) | ((uint32_t)pixel.green << 8) | (uint32_t)pixel.blue;
    }
    inline Pixel RGBValue(uint32_t hexValue)
    {
        Pixel pixel;
        pixel.red = (hexValue >> 16) & 0xFF;
        pixel.green = (hexValue >> 8) & 0xFF;
        pixel.blue = hexValue & 0xFF;
        return pixel;
    }
} // namespace NeoLED

#endif // NEOLED_H
