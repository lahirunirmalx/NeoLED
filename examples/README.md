# NeoLED Examples

Each folder is a standalone `app_main` you can drop into an ESP-IDF / PlatformIO
project (or copy into your own `main.cpp`).

| Example | What it shows |
|---------|---------------|
| [`01_single_led`](01_single_led/main.cpp) | One WS2812 on the default pin, "breathing" white via `updateWithBrightness()`. |
| [`02_strip_segments`](02_strip_segments/main.cpp) | Several strips wired **in series** and addressed as index ranges of one logical array. |
| [`03_matrix`](03_matrix/main.cpp) | An 8×8 matrix with a serpentine `xy()` mapping and a scrolling HSV rainbow. |
| [`04_parallel_strips`](04_parallel_strips/main.cpp) | Two **independent** strips on the two I2S ports, refreshed together with `updateParallel()`. |
| [`05_multicore`](05_multicore/main.cpp) | One strip per CPU core — a `NeoLED::Strip` instance driven from a task pinned to each core. |

## Building one

The LED count is a compile-time setting, so define it **before** including the
header (the single-LED example uses the default of 1):

```cpp
#define LED_NUMBER 24   // total LEDs on the chain
#include "neoled.h"
```

Point the data line at your wiring with `I2S_DO_IO` (compile-time) or
`NeoLED::initWithPin(gpio)` (runtime).

## Two APIs: default strip vs. Strip instances

- **Free functions** (`NeoLED::init/update/...`) drive one built-in default
  strip whose LED count is the compile-time `LED_NUMBER`. Simplest for a single
  strip — examples 01–03 use it.
- **`NeoLED::Strip`** is an instance you create per data line, with a **runtime**
  LED count and a chosen **I2S port**. Use it for more than one strip, parallel
  updates, or per-core rendering — examples 04–05.

### One data line per channel

Each I2S peripheral drives **one data line**. So "multiple strips" / "matrix" on
a single channel means *one chain* of LEDs:

- **Chained strips** — wire each strip's `DOUT` into the next strip's `DIN`; the
  whole run is one array, each strip a slice of it (see `02_strip_segments`).
- **Matrix** — `width * height` LEDs with an `xy()` helper (see `03_matrix`).

For genuinely **independent** strips, give each its own I2S port (ESP32 / S3
have two) via separate `Strip` instances and `updateParallel()` — see
`04_parallel_strips`. The number of parallel strips is capped by the SoC's I2S
peripheral count.
