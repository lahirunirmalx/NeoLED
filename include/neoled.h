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
#ifndef NEOLED_H
#define NEOLED_H

#include <cstdint>

// Library version
#define NEOLED_VERSION_MAJOR 1
#define NEOLED_VERSION_MINOR 1
#define NEOLED_VERSION_PATCH 0
#define NEOLED_VERSION_STRING "1.1.0"

// ESP-IDF version detection for compatibility
#include "esp_idf_version.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    #define NEOLED_USE_NEW_I2S_DRIVER 1
#else
    #define NEOLED_USE_NEW_I2S_DRIVER 0
#endif

namespace NeoLED {

// ============================================================================
// Configuration Macros (can be overridden before including this header)
// ============================================================================

#ifndef LED_NUMBER
    #define LED_NUMBER 1
#endif

#ifndef PIXEL_SIZE
    #define PIXEL_SIZE 12  // Each color takes 4 bytes (RGB = 12 bytes total)
#endif

#ifndef SAMPLE_RATE
    #define SAMPLE_RATE (93750)
#endif

#ifndef ZERO_BUFFER
    #define ZERO_BUFFER 48
#endif

#ifndef I2S_NUM
    #define I2S_NUM (0)
#endif

#ifndef I2S_DO_IO
    #define I2S_DO_IO (21)
#endif

// ============================================================================
// Error Codes
// ============================================================================

typedef enum {
    NEOLED_OK = 0,              // Success
    NEOLED_ERR_INIT = -1,       // Initialization failed
    NEOLED_ERR_PARAM = -2,      // Invalid parameter
    NEOLED_ERR_NO_MEM = -3,     // Memory allocation failed
    NEOLED_ERR_NOT_INIT = -4,   // Not initialized
    NEOLED_ERR_I2S = -5         // I2S operation failed
} neoled_err_t;

// ============================================================================
// Pixel Structure
// ============================================================================

/**
 * @brief Pixel structure for RGB LED data
 * @note WS2812 uses GRB format, so the order is green, red, blue
 */
typedef struct {
    uint8_t green;
    uint8_t red;
    uint8_t blue;
} Pixel;

/**
 * @brief Extended pixel structure for RGBW LEDs (future use)
 */
typedef struct {
    uint8_t green;
    uint8_t red;
    uint8_t blue;
    uint8_t white;
} PixelW;

// ============================================================================
// Predefined Colors (in RGB order for user convenience)
// These create Pixel structs with correct GRB internal ordering
// ============================================================================

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

// ============================================================================
// Predefined Hue Values (0-255 range)
// ============================================================================

#define HUE_RED        0      // Red
#define HUE_ORANGE     32     // Orange
#define HUE_YELLOW     64     // Yellow
#define HUE_LIME       80     // Lime Green
#define HUE_GREEN      96     // Green
#define HUE_TURQUOISE  112    // Turquoise
#define HUE_CYAN       128    // Cyan
#define HUE_AQUA       144    // Aqua Blue
#define HUE_BLUE       160    // Blue
#define HUE_PURPLE     176    // Purple
#define HUE_MAGENTA    192    // Magenta (Pink)
#define HUE_ROSE       224    // Rose Pink
#define HUE_WHITE      0      // Use full brightness for white (no hue shift)
#define HUE_OFF        0      // Turn off LED (set RGB values to zero)

// ============================================================================
// Core Functions
// ============================================================================

/**
 * @brief Initialize NeoLED with default configuration
 * @return NEOLED_OK on success, error code otherwise
 */
neoled_err_t init(void);

/**
 * @brief Initialize NeoLED with custom GPIO pin
 * @param gpio_pin GPIO pin number for data output
 * @return NEOLED_OK on success, error code otherwise
 */
neoled_err_t initWithPin(int gpio_pin);

/**
 * @brief Update LED strip with pixel data
 * @param pixels Pointer to pixel array
 * @return NEOLED_OK on success, error code otherwise
 */
neoled_err_t update(const Pixel* pixels);

/**
 * @brief Update LED strip with pixel data and brightness adjustment
 * @param pixels Pointer to pixel array
 * @param brightness Global brightness (0-255)
 * @return NEOLED_OK on success, error code otherwise
 */
neoled_err_t updateWithBrightness(const Pixel* pixels, uint8_t brightness);

/**
 * @brief Turn off all LEDs
 * @return NEOLED_OK on success, error code otherwise
 */
neoled_err_t clear(void);

/**
 * @brief Release I2S resources and reset GPIO
 * @return NEOLED_OK on success, error code otherwise
 */
neoled_err_t destroy(void);

/**
 * @brief Check if NeoLED is initialized
 * @return true if initialized, false otherwise
 */
bool isInitialized(void);

/**
 * @brief Set global brightness level
 * @param brightness Brightness level (0-255, default 255)
 */
void setBrightness(uint8_t brightness);

/**
 * @brief Get current global brightness level
 * @return Current brightness (0-255)
 */
uint8_t getBrightness(void);

// ============================================================================
// Pixel Creation Functions (Inline for performance)
// ============================================================================

/**
 * @brief Create a pixel from RGB values
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @return Pixel structure
 */
inline Pixel makePixel(uint8_t r, uint8_t g, uint8_t b)
{
    Pixel pixel;
    pixel.red = r;
    pixel.green = g;
    pixel.blue = b;
    return pixel;
}

/**
 * @brief Create a pixel with brightness adjustment
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @param brightness Brightness level (0-255)
 * @return Pixel structure with adjusted brightness
 */
inline Pixel makePixelWithBrightness(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    Pixel pixel;
    pixel.red = (uint8_t)((r * brightness) / 255);
    pixel.green = (uint8_t)((g * brightness) / 255);
    pixel.blue = (uint8_t)((b * brightness) / 255);
    return pixel;
}

/**
 * @brief Generate a color from the color wheel (rainbow effect)
 * @param hue Hue value (0-255)
 * @return Pixel with the corresponding color
 */
inline Pixel colorWheel(uint8_t hue)
{
    Pixel pixel;

    if (hue < 85) {
        pixel.red = hue * 3;
        pixel.green = 255 - hue * 3;
        pixel.blue = 0;
    } else if (hue < 170) {
        hue -= 85;
        pixel.red = 255 - hue * 3;
        pixel.green = 0;
        pixel.blue = hue * 3;
    } else {
        hue -= 170;
        pixel.red = 0;
        pixel.green = hue * 3;
        pixel.blue = 255 - hue * 3;
    }

    return pixel;
}

/**
 * @brief Generate a color from HSV values
 * @param h Hue (0-255)
 * @param s Saturation (0-255)
 * @param v Value/Brightness (0-255)
 * @return Pixel with the corresponding color
 */
inline Pixel fromHSV(uint8_t h, uint8_t s, uint8_t v)
{
    Pixel pixel;
    
    if (s == 0) {
        // Achromatic (grey)
        pixel.red = pixel.green = pixel.blue = v;
        return pixel;
    }

    uint8_t region = h / 43;
    uint8_t remainder = (h - (region * 43)) * 6;

    uint8_t p = (v * (255 - s)) >> 8;
    uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:
            pixel.red = v; pixel.green = t; pixel.blue = p;
            break;
        case 1:
            pixel.red = q; pixel.green = v; pixel.blue = p;
            break;
        case 2:
            pixel.red = p; pixel.green = v; pixel.blue = t;
            break;
        case 3:
            pixel.red = p; pixel.green = q; pixel.blue = v;
            break;
        case 4:
            pixel.red = t; pixel.green = p; pixel.blue = v;
            break;
        default:
            pixel.red = v; pixel.green = p; pixel.blue = q;
            break;
    }

    return pixel;
}

/**
 * @brief Calculate approximate hue value from a pixel
 * @param pixel Source pixel
 * @return Hue value (0-255)
 * @note This is an approximation and may not be exact for all colors
 */
inline uint8_t hueValue(const Pixel& pixel)
{
    uint8_t maxVal = pixel.red;
    uint8_t minVal = pixel.red;
    
    if (pixel.green > maxVal) maxVal = pixel.green;
    if (pixel.blue > maxVal) maxVal = pixel.blue;
    if (pixel.green < minVal) minVal = pixel.green;
    if (pixel.blue < minVal) minVal = pixel.blue;

    if (maxVal == minVal) {
        return 0;  // Achromatic
    }

    uint8_t delta = maxVal - minVal;
    uint16_t hue = 0;

    if (maxVal == pixel.red) {
        hue = 43 * (pixel.green - pixel.blue) / delta;
    } else if (maxVal == pixel.green) {
        hue = 85 + 43 * (pixel.blue - pixel.red) / delta;
    } else {
        hue = 171 + 43 * (pixel.red - pixel.green) / delta;
    }

    return (uint8_t)hue;
}

/**
 * @brief Convert pixel to 32-bit hex value (0xRRGGBB)
 * @param pixel Source pixel
 * @return 32-bit hex color value
 */
inline uint32_t hexValue(const Pixel& pixel)
{
    return ((uint32_t)pixel.red << 16) | ((uint32_t)pixel.green << 8) | (uint32_t)pixel.blue;
}

/**
 * @brief Create a pixel from 32-bit hex value (0xRRGGBB)
 * @param hexVal 32-bit hex color value
 * @return Pixel structure
 */
inline Pixel fromHex(uint32_t hexVal)
{
    Pixel pixel;
    pixel.red = (hexVal >> 16) & 0xFF;
    pixel.green = (hexVal >> 8) & 0xFF;
    pixel.blue = hexVal & 0xFF;
    return pixel;
}

/**
 * @brief Create a pixel from 32-bit hex value (0xRRGGBB)
 * @param hexVal 32-bit hex color value
 * @return Pixel structure
 * @deprecated Use fromHex() instead
 */
inline Pixel RGBValue(uint32_t hexVal)
{
    return fromHex(hexVal);
}

/**
 * @brief Blend two pixels together
 * @param a First pixel
 * @param b Second pixel
 * @param blend Blend amount (0 = all a, 255 = all b)
 * @return Blended pixel
 */
inline Pixel blend(const Pixel& a, const Pixel& b, uint8_t blendAmount)
{
    Pixel pixel;
    pixel.red = (uint8_t)((a.red * (255 - blendAmount) + b.red * blendAmount) / 255);
    pixel.green = (uint8_t)((a.green * (255 - blendAmount) + b.green * blendAmount) / 255);
    pixel.blue = (uint8_t)((a.blue * (255 - blendAmount) + b.blue * blendAmount) / 255);
    return pixel;
}

/**
 * @brief Apply gamma correction to a pixel
 * @param pixel Source pixel
 * @param gamma Gamma value (typically 2.2-2.8)
 * @return Gamma-corrected pixel
 */
Pixel gammaCorrect(const Pixel& pixel, float gamma = 2.2f);

} // namespace NeoLED

#endif // NEOLED_H
