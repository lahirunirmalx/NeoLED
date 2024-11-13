# NeoLED - ESP32 Component for WS2812 LEDs Using I2S

## Introduction

NeoLED is an ESP32 component library designed specifically for controlling WS2812 NeoPixel LEDs using the I2S peripheral of the ESP-IDF SDK. This library was created to fill the gap when a suitable existing library was not found, particularly for use in the M5Stack Cardputer, where GPIO control via I2S is essential for reliable LED performance.

### Why Use I2S for NeoPixels?

The WS2812 LEDs typically rely on precise timing signals, which can be challenging to achieve with regular GPIO operations, especially on the ESP32 when running other tasks concurrently. By leveraging the I2S peripheral, NeoLED can generate the necessary timing signals more accurately, reducing flicker and glitches even under heavy CPU load.

## Features

- **Reliable I2S Control**: Ensures stable operation of WS2812 LEDs by using the I2S peripheral for precise timing.
- **Customizable GPIO**: Defaults to GPIO 21, but can be configured via `neoled.h`.
- **Simple API**: Easy-to-use API for initializing, setting pixel colors, and updating the LED strip.
- **Hue Support**: Includes a color wheel function for smooth hue transitions.

## Installation

1. Copy the `NeoLED` component folder into your ESP-IDF project under the `components` directory.
2. Add `NeoLED` to your project's `CMakeLists.txt`:

```cmake
set(COMPONENT_SRCS "ws2812.cpp")
set(COMPONENT_ADD_INCLUDEDIRS "include")
register_component()
```

3. Include the library in your source files:

```cpp
#include "neoled.h"
```

## Configuration

You can change the default settings in `neoled.h`:

```cpp
#define LED_NUMBER 1          // Number of LEDs in your strip
#define I2S_DO_IO GPIO_NUM_21 // Default GPIO pin for data output
```

## Usage

Below is an example of how to use NeoLED in your ESP-IDF project:

```cpp
#include "neoled.h"

extern "C" void app_main() {
    // Initialize NeoLED
    NeoLED::init();

    // Create a green pixel
    NeoLED::Pixel green_pixel = NeoLED::makePixel(0, 255, 0);

    // Update the LED with the green pixel
    NeoLED::update(&green_pixel);

    // Example with hue color wheel
    for (int hue = 0; hue < 256; hue++) {
        NeoLED::Pixel pixel = NeoLED::colorWheel(hue);
        NeoLED::update(&pixel);
        vTaskDelay(pdMS_TO_TICKS(50)); // Delay for smooth transition
    }
}
```

### Explanation:

- **`NeoLED::init()`**: Initializes the I2S peripheral for controlling the LEDs.
- **`NeoLED::makePixel(r, g, b)`**: Creates a pixel with specified RGB values.
- **`NeoLED::update(pixel)`**: Updates the LED strip with the pixel data.
- **`NeoLED::colorWheel(hue)`**: Generates a pixel color based on a hue value (0-255).

### Advanced Usage

You can customize the LED configuration directly in `neoled.h`, or implement your own memory allocation for dynamically sized LED strips.

## Known Issues

- **Limited GPIO Compatibility**: The library defaults to GPIO 21, which is suitable for M5Stack Cardputer. If using other hardware, ensure the chosen GPIO pin supports I2S output.
- **Single LED Support**: Currently designed for up to one LED. Future updates will include support for longer LED strips and RGBW LEDs.

## Planned Improvements

- **Support for RGBW LEDs**: Add functionality to handle RGBW NeoPixel strips.
- **Dynamic Configuration**: Implement initialization functions to allow dynamic configuration of LED numbers and GPIO pins without modifying the header file.
- **Memory Management**: Add custom memory allocation options for better control over resource usage.

## Debugging Tips

- **LED Not Lighting Up**: Ensure that the data pin (`I2S_DO_IO`) is correctly configured and connected to the input of the LED strip.
- **Flickering LEDs**: This may be due to incorrect power supply or timing issues. Verify that the power supply can handle the current draw of the LEDs.
- **Incorrect Colors**: Check the RGB order. Some NeoPixel strips use different color orders (e.g., GRB instead of RGB). Modify `makePixel()` accordingly.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributions

Contributions are welcome! Please feel free to submit pull requests or open issues for any bugs or feature requests. If you create a new app or feature for the M5Stack Cardputer using this library, consider sharing it with the community!

## Acknowledgments

Special thanks to [Vu Nam](https://github.com/vunam) for the original inspiration and implementation of a WS2812 I2S driver for the ESP32. This project builds on those efforts and aims to provide a robust solution for the M5Stack Cardputer.

---

