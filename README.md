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

### ESP-IDF Component Manager (recommended)

Add the dependency to your project (or run the command):

```bash
idf.py add-dependency "lahirunirmalx/neoled^1.4.0"
```

…or copy the folder into your project's `components/` directory. Then:

```cpp
#include "neoled.h"      // C++ API
// or
#include "neoled_c.h"    // C API
```

### PlatformIO

Add to `platformio.ini`:

```ini
lib_deps = https://github.com/lahirunirmalx/NeoLED.git
```

…or copy the folder into your project's `lib/` directory, then `#include "neoled.h"`.

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

// Introspection
uint16_t NeoLED::numLeds(void);      // LED count the library was built for
int NeoLED::getGpioPin(void);        // active data GPIO (valid after init)
const char* NeoLED::version(void);   // e.g. "1.2.0"
```

> **Thread safety:** The default free-function API is mutex-protected, so
> `update()` / `clear()` are safe to call from multiple tasks. For independent
> strips driven concurrently, prefer separate `Strip` instances (below).

### Multiple Strips, Parallel & Multicore (`Strip` class)

For more than one independent strip — one per I2S peripheral, refreshed in
parallel and/or from different cores — create a `NeoLED::Strip` per data line.
Each instance owns its I2S port, frame buffer, brightness, and mutex.

```cpp
namespace NeoLED {
class Strip {
public:
    // Initialize on a GPIO + I2S port with a runtime LED count.
    neoled_err_t begin(int gpio_pin, uint16_t led_count, int i2s_port = I2S_NUM);
    neoled_err_t end(void);
    bool isInitialized(void) const;

    neoled_err_t update(const Pixel* pixels);
    neoled_err_t updateWithBrightness(const Pixel* pixels, uint8_t brightness);
    neoled_err_t clear(void);

    void     setBrightness(uint8_t brightness);
    uint8_t  getBrightness(void) const;
    uint16_t numLeds(void) const;
    int      getGpioPin(void) const;
    int      getPort(void) const;
};

// Update several strips so their DMA transfers overlap (refresh together).
neoled_err_t updateParallel(Strip* const* strips,
                            const Pixel* const* pixels,
                            uint8_t count);
}
```

```cpp
NeoLED::Strip s0, s1;
s0.begin(21, 16, /*i2s_port=*/0);
s1.begin(22, 16, /*i2s_port=*/1);

NeoLED::Strip* strips[]       = { &s0, &s1 };
const NeoLED::Pixel* frames[] = { frame0, frame1 };
NeoLED::updateParallel(strips, frames, 2);   // both strips light up together
```

Notes:
- The number of parallel strips is limited by the SoC's I2S peripheral count
  (ESP32 / ESP32-S3: 2; S2 / C3 / C6 / H2: 1). `begin()` returns
  `NEOLED_ERR_PARAM` for an out-of-range port.
- `begin()` allocates a `led_count * 12`-byte buffer; it returns
  `NEOLED_ERR_NO_MEM` if allocation fails.
- Call `updateParallel()` from a single coordinator task and pass **distinct**
  strips (passing one twice would deadlock its mutex).
- For per-core rendering, give each core its own `Strip` and task — see
  [`examples/05_multicore`](examples/05_multicore/main.cpp).

### Color order, RGBW & per-pixel API

```cpp
NeoLED::Strip strip;
strip.begin(18, 30, /*port=*/0, NeoLED::ORDER_RGB, /*rgbw=*/true);  // WS2811-order RGBW

strip.fill(NeoLED::COLOR_OFF);
strip.setPixel(0, NeoLED::COLOR_RED);                 // RGB pixel
strip.setPixelW(1, NeoLED::PixelW{0, 0, 0, 255});     // pure white channel
strip.show();                                         // render framebuffer + transmit
```

`setPixel`/`fill`/`fillRange` write the internal framebuffer (build a frame from
one task), and `show()` renders + transmits. `getPixel(i)` reads it back.

### C API (`neoled_c.h`)

For plain-C projects — handle-based, no C++ required:

```c
#include "neoled_c.h"

neoled_strip_handle_t s = neoled_strip_create();
neoled_strip_begin(s, 21, 8, 0);          // gpio, count, i2s_port
neoled_strip_set_pixel(s, 0, 255, 0, 0);
neoled_strip_show(s);
neoled_strip_destroy(s);
```

See [`examples/06_c_api`](examples/06_c_api/main.c). Int-returning functions give
`0` on success or a negative error code.

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

> Runnable, copy-paste examples live in [`examples/`](examples/): a
> [single LED](examples/01_single_led/main.cpp), multiple
> [chained strips](examples/02_strip_segments/main.cpp), and an
> [8×8 matrix](examples/03_matrix/main.cpp).

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

### v1.4.0
- **Configurable color order** (`ORDER_GRB`/`RGB`/`BRG`/`RBG`/`GBR`/`BGR`) per strip — drive WS2811/PL9823/APA106 (RGB-order clones) without swapping channels yourself.
- **RGBW support** (e.g. SK6812-RGBW): `Strip::begin(..., rgbw=true)`, `updateW()`, `setPixelW()`.
- **Per-pixel framebuffer API**: `setPixel()`, `setPixelW()`, `getPixel()`, `fill()`, `fillRange()`, `show()` — with bounds checks.
- **C API** (`neoled_c.h`): handle-based functions so plain-C ESP-IDF projects can use the library without C++.
- **Deterministic latch**: replaced the config-dependent `vTaskDelay(1)` with `esp_rom_delay_us(280)`, fixing refresh rate and reset timing.
- **Packaging & CI**: ESP Component Registry manifest (`idf_component.yml`), PlatformIO `library.json`, modernized `CMakeLists.txt`, GitHub Actions building across ESP-IDF 4.4–5.3 × esp32/s3/c3, plus host unit tests for the color math.
- Backward compatible: the existing C++ free functions and `Strip` calls are unchanged (new `begin()` args are defaulted).

### v1.3.0
- **Multiple I2S channels / parallel strips.** New `NeoLED::Strip` class — one instance per data line, each with its own I2S port, frame buffer, brightness, and mutex. Drive several independent strips at once with `NeoLED::updateParallel()` (up to the SoC's I2S peripheral count: 2 on ESP32 / ESP32-S3, 1 on S2/C3/C6/H2).
- **Multicore-safe.** Each `Strip` is independent and mutex-protected, so different strips can be rendered from tasks pinned to different cores (see `examples/05_multicore`). The default free-function API is now mutex-protected too.
- **Runtime LED count.** `Strip::begin(gpio, led_count, port)` sizes its buffer at runtime (heap-allocated), so LED count no longer has to be a compile-time constant for the instance API.
- Backward compatible: the existing free functions (`init`, `update`, `clear`, …) are unchanged and now simply wrap a built-in default `Strip`.
- Added `examples/04_parallel_strips` and `examples/05_multicore`.

### v1.2.0
- Predefined `COLOR_*` and `HUE_*` are now namespaced `constexpr` constants instead of macros, so both `NeoLED::COLOR_RED` and (with `using namespace NeoLED`) `COLOR_RED` compile. Same names and values — existing code keeps working.
- Added `numLeds()`, `getGpioPin()`, and `version()` accessors.
- `clear()` no longer allocates a per-LED array on the stack (safe for large `LED_NUMBER`).
- Refactored the I2S transmit path into a single shared helper, removing duplicated write logic between `update()` and `clear()`.
- Added the missing `LICENSE` file and fixed `.gitignore` (was `.gitignore.txt`).
- Added runnable examples under `examples/` (single LED, chained strips, 8×8 matrix) and a "Compatible LED Chips" reference for WS2812 clones/variants.

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

## Compatible LED Chips

NeoLED generates a **single-wire ~800 kHz NRZ** signal with **configurable
color order** and optional **RGBW** (4th byte). Any addressable LED that speaks
that protocol works — these are the common WS2812 "clones" and relatives:

| Chip | Works | Notes |
|------|-------|-------|
| WS2812 / WS2812B / WS2812C / WS2812D | ✅ | Reference part this library targets. |
| WS2813 | ✅ | Dual-signal (backup data) version, same protocol. |
| WS2815 | ✅ | 12 V strip, same data timing/order — just power it from 12 V. |
| SK6812 (RGB) | ✅ | WS2812-compatible timing and GRB order. |
| SK6805 | ✅ | Smaller SK6812 family member. |
| WS2811 | ✅ | 800 kHz mode. Usually **RGB** order — pass `ORDER_RGB` to `begin()`. |
| PL9823 | ✅ | 800 kHz single-wire, **RGB** order — pass `ORDER_RGB`. |
| APA106 | ✅ | Through-hole, **RGB** order — pass `ORDER_RGB`. |
| SK6812-**RGBW** | ✅ | Pass `rgbw=true` to `begin()`; use `updateW()` / `setPixelW()`. |
| APA102 / SK9822 ("DotStar") | ❌ | Two-wire **SPI** (clock + data), a different protocol. |
| WS2801, LPD8806 | ❌ | Also clocked SPI parts, not single-wire. |

**Color order:** pick the order that matches your chip at `begin()` time
(default `ORDER_GRB`); the library stores logical R/G/B and reorders on the
wire, so your `makePixel(r, g, b)` values are always correct.

## Known Issues

- **Limited GPIO Compatibility**: The library defaults to GPIO 21, which is suitable for M5Stack Cardputer. If using other hardware, ensure the chosen GPIO pin supports I2S output.
- **Static LED Count (default API only)**: The free-function API uses the compile-time `LED_NUMBER`. For a runtime-sized strip, use the `NeoLED::Strip` class, whose `begin(gpio, led_count, port)` allocates its buffer dynamically.

## Planned Improvements

- **Animation Framework**: Built-in effects like breathing, chase, fade, etc.
- **Non-blocking / double-buffered updates** for high-FPS animation.

> Done in v1.3.0: runtime LED count and multiple parallel I2S channels via the `NeoLED::Strip` class.
> Done in v1.4.0: configurable color order, RGBW, per-pixel API, and a C API.

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

