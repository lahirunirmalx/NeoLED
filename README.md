# NeoLED - ESP32 Component for WS2812 LEDs Using I2S

[![Version](https://img.shields.io/badge/version-1.1.0-blue.svg)](https://github.com/yourusername/neoled)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-4.x%20%7C%205.x-orange.svg)](https://docs.espressif.com/projects/esp-idf/)

## Introduction

NeoLED is an ESP32 component library designed specifically for controlling WS2812 NeoPixel LEDs using the I2S peripheral of the ESP-IDF SDK. This library was created to fill the gap when a suitable existing library was not found, particularly for use in the M5Stack Cardputer, where GPIO control via I2S is essential for reliable LED performance.

### Why Use I2S for NeoPixels?

The WS2812 LEDs typically rely on precise timing signals, which can be challenging to achieve with regular GPIO operations, especially on the ESP32 when running other tasks concurrently. By leveraging the I2S peripheral, NeoLED can generate the necessary timing signals more accurately, reducing flicker and glitches even under heavy CPU load.

## Features

- **ESP-IDF 4.x & 5.x Compatible**: Automatic detection and support for both legacy and new I2S drivers
- **Reliable I2S Control**: Ensures stable operation of WS2812 LEDs using I2S peripheral for precise timing
- **Customizable GPIO**: Defaults to GPIO 21, configurable at compile-time or runtime
- **Brightness Control**: Global brightness setting with per-update brightness override
- **Color Utilities**: HSV conversion, color wheel, gamma correction, and color blending
- **Error Handling**: Comprehensive error codes and logging for easier debugging
- **Simple API**: Easy-to-use API for initializing, setting pixel colors, and updating the LED strip

## Installation

### PlatformIO

1. Copy the `NeoLED` component folder into your project's `lib` or `components` directory.

2. Include the library in your source files:

```cpp
#include "neoled.h"
```

### ESP-IDF

1. Copy the `NeoLED` component folder into your ESP-IDF project under the `components` directory.

2. The component will be automatically detected and included by the build system.

3. Include the library in your source files:

```cpp
#include "neoled.h"
```

## Configuration

You can override the default settings by defining macros **before** including `neoled.h`:

```cpp
// Override defaults before including the header
#define LED_NUMBER 8           // Number of LEDs in your strip
#define I2S_DO_IO 21           // GPIO pin for data output

#include "neoled.h"
```

### Available Configuration Options

| Macro | Default | Description |
|-------|---------|-------------|
| `LED_NUMBER` | 1 | Number of LEDs in your strip |
| `I2S_DO_IO` | 21 | GPIO pin for data output |
| `I2S_NUM` | 0 | I2S peripheral number (0 or 1) |
| `SAMPLE_RATE` | 93750 | I2S sample rate for WS2812 timing |
| `PIXEL_SIZE` | 12 | Bytes per pixel (do not change) |
| `ZERO_BUFFER` | 48 | Reset signal buffer size |

## API Reference

### Initialization & Cleanup

```cpp
// Initialize with default GPIO (I2S_DO_IO)
NeoLED::neoled_err_t NeoLED::init(void);

// Initialize with custom GPIO pin
NeoLED::neoled_err_t NeoLED::initWithPin(int gpio_pin);

// Check if initialized
bool NeoLED::isInitialized(void);

// Release resources
NeoLED::neoled_err_t NeoLED::destroy(void);
```

### LED Control

```cpp
// Update LEDs with pixel data
NeoLED::neoled_err_t NeoLED::update(const Pixel* pixels);

// Update with specific brightness (0-255)
NeoLED::neoled_err_t NeoLED::updateWithBrightness(const Pixel* pixels, uint8_t brightness);

// Turn off all LEDs
NeoLED::neoled_err_t NeoLED::clear(void);

// Set/get global brightness
void NeoLED::setBrightness(uint8_t brightness);
uint8_t NeoLED::getBrightness(void);
```

### Pixel Creation

```cpp
// Create pixel from RGB values
Pixel NeoLED::makePixel(uint8_t r, uint8_t g, uint8_t b);

// Create pixel with brightness adjustment
Pixel NeoLED::makePixelWithBrightness(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);

// Create from HSV values
Pixel NeoLED::fromHSV(uint8_t h, uint8_t s, uint8_t v);

// Color wheel (rainbow effect)
Pixel NeoLED::colorWheel(uint8_t hue);

// From/to hex values (0xRRGGBB)
Pixel NeoLED::fromHex(uint32_t hexVal);
uint32_t NeoLED::hexValue(const Pixel& pixel);
```

### Color Utilities

```cpp
// Blend two colors
Pixel NeoLED::blend(const Pixel& a, const Pixel& b, uint8_t blendAmount);

// Gamma correction (default gamma = 2.2)
Pixel NeoLED::gammaCorrect(const Pixel& pixel, float gamma = 2.2f);

// Get approximate hue from pixel
uint8_t NeoLED::hueValue(const Pixel& pixel);
```

### Error Codes

| Code | Value | Description |
|------|-------|-------------|
| `NEOLED_OK` | 0 | Success |
| `NEOLED_ERR_INIT` | -1 | Initialization failed |
| `NEOLED_ERR_PARAM` | -2 | Invalid parameter |
| `NEOLED_ERR_NO_MEM` | -3 | Memory allocation failed |
| `NEOLED_ERR_NOT_INIT` | -4 | Not initialized |
| `NEOLED_ERR_I2S` | -5 | I2S operation failed |

### Predefined Colors

```cpp
COLOR_RED, COLOR_ORANGE, COLOR_YELLOW, COLOR_LIME,
COLOR_GREEN, COLOR_TURQUOISE, COLOR_CYAN, COLOR_AQUA,
COLOR_BLUE, COLOR_PURPLE, COLOR_MAGENTA, COLOR_ROSE,
COLOR_WHITE, COLOR_OFF
```

## Usage Examples

### Basic Usage

```cpp
#include "neoled.h"

extern "C" void app_main() {
    // Initialize NeoLED
    if (NeoLED::init() != NeoLED::NEOLED_OK) {
        printf("Failed to initialize NeoLED\n");
        return;
    }

    // Create a green pixel
    NeoLED::Pixel green_pixel = NeoLED::makePixel(0, 255, 0);

    // Update the LED with the green pixel
    NeoLED::update(&green_pixel);

    // Cleanup when done
    NeoLED::destroy();
}
```

### Rainbow Effect

```cpp
#include "neoled.h"

extern "C" void app_main() {
    NeoLED::init();

    // Cycle through the color wheel
    while (true) {
        for (int hue = 0; hue < 256; hue++) {
            NeoLED::Pixel pixel = NeoLED::colorWheel(hue);
            NeoLED::update(&pixel);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}
```

### Brightness Control

```cpp
#include "neoled.h"

extern "C" void app_main() {
    NeoLED::init();
    
    // Set global brightness to 50%
    NeoLED::setBrightness(128);
    
    NeoLED::Pixel white = NeoLED::COLOR_WHITE;
    NeoLED::update(&white);

    // Or use per-update brightness
    NeoLED::updateWithBrightness(&white, 64);  // 25% brightness
}
```

### HSV Colors

```cpp
#include "neoled.h"

extern "C" void app_main() {
    NeoLED::init();

    // Create color from HSV (Hue, Saturation, Value)
    // Full saturation, 50% brightness
    NeoLED::Pixel pixel = NeoLED::fromHSV(128, 255, 128);
    NeoLED::update(&pixel);
}
```

### Color Blending

```cpp
#include "neoled.h"

extern "C" void app_main() {
    NeoLED::init();

    NeoLED::Pixel red = NeoLED::COLOR_RED;
    NeoLED::Pixel blue = NeoLED::COLOR_BLUE;
    
    // Blend 50% red + 50% blue = purple
    NeoLED::Pixel purple = NeoLED::blend(red, blue, 128);
    NeoLED::update(&purple);
}
```

### Gamma Correction

```cpp
#include "neoled.h"

extern "C" void app_main() {
    NeoLED::init();

    // Apply gamma correction for more natural color perception
    NeoLED::Pixel pixel = NeoLED::makePixel(128, 128, 128);
    NeoLED::Pixel corrected = NeoLED::gammaCorrect(pixel);
    NeoLED::update(&corrected);
}
```

### Custom GPIO Pin

```cpp
#include "neoled.h"

extern "C" void app_main() {
    // Initialize with custom GPIO pin
    if (NeoLED::initWithPin(25) != NeoLED::NEOLED_OK) {
        printf("Failed to initialize on GPIO 25\n");
        return;
    }

    NeoLED::Pixel pixel = NeoLED::COLOR_GREEN;
    NeoLED::update(&pixel);
}
```

## Changelog

### v1.1.0
- Added ESP-IDF 5.x support with automatic version detection
- Added `initWithPin()` for runtime GPIO configuration
- Added brightness control (`setBrightness()`, `getBrightness()`, `updateWithBrightness()`)
- Added `clear()` function to turn off all LEDs
- Added `isInitialized()` status check
- Added HSV color support with `fromHSV()`
- Added color blending with `blend()`
- Added gamma correction with `gammaCorrect()`
- Added comprehensive error codes and logging
- Improved `hueValue()` algorithm for better accuracy
- Added `fromHex()` function (replaces `RGBValue()`, kept for backward compatibility)
- Added `makePixelWithBrightness()` utility
- Fixed inconsistent code formatting
- Updated documentation with full API reference

### v1.0.0
- Initial release
- Basic WS2812 LED control via I2S
- Color wheel and basic pixel utilities

## Known Issues

- **Limited GPIO Compatibility**: The library defaults to GPIO 21, which is suitable for M5Stack Cardputer. If using other hardware, ensure the chosen GPIO pin supports I2S output.
- **Static LED Count**: The number of LEDs is set at compile-time via `LED_NUMBER`. Dynamic allocation is planned for a future release.

## Planned Improvements

- **Support for RGBW LEDs**: Add functionality to handle RGBW NeoPixel strips (PixelW struct already defined).
- **Dynamic LED Count**: Allow changing LED count at runtime without recompilation.
- **Animation Framework**: Built-in effects like breathing, chase, fade, etc.

## Debugging Tips

- **LED Not Lighting Up**: Ensure that the data pin is correctly configured and connected to the input of the LED strip. Check the return value of `init()`.
- **Flickering LEDs**: This may be due to incorrect power supply or timing issues. Verify that the power supply can handle the current draw of the LEDs.
- **Incorrect Colors**: Check the RGB order. WS2812 uses GRB format internally, but the API accepts standard RGB values.
- **Enable Logging**: The library uses ESP-IDF logging. Set log level to DEBUG to see detailed information.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributions

Contributions are welcome! Please feel free to submit pull requests or open issues for any bugs or feature requests. If you create a new app or feature for the M5Stack Cardputer using this library, consider sharing it with the community!

## Acknowledgments

Special thanks to [Vu Nam](https://github.com/vunam) for the original inspiration and implementation of a WS2812 I2S driver for the ESP32. This project builds on those efforts and aims to provide a robust solution for the M5Stack Cardputer.

---

