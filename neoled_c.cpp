/** MIT licence — see LICENSE.

 Implementation of the NeoLED C API: a thin shim over NeoLED::Strip.
*/

#include <new>
#include "neoled.h"
#include "neoled_c.h"

using NeoLED::Strip;
using NeoLED::Pixel;
using NeoLED::PixelW;

static inline Strip* as_strip(neoled_strip_handle_t h)
{
    return reinterpret_cast<Strip*>(h);
}

static inline NeoLED::ColorOrder as_order(neoled_color_order_t o)
{
    return static_cast<NeoLED::ColorOrder>(o);
}

extern "C" {

neoled_strip_handle_t neoled_strip_create(void)
{
    return reinterpret_cast<neoled_strip_handle_t>(new (std::nothrow) Strip());
}

void neoled_strip_destroy(neoled_strip_handle_t handle)
{
    delete as_strip(handle);
}

int neoled_strip_begin(neoled_strip_handle_t handle, int gpio, uint16_t led_count, int i2s_port)
{
    if (!handle) return NeoLED::NEOLED_ERR_PARAM;
    return as_strip(handle)->begin(gpio, led_count, i2s_port);
}

int neoled_strip_begin_ex(neoled_strip_handle_t handle, int gpio, uint16_t led_count,
                          int i2s_port, neoled_color_order_t order, bool rgbw)
{
    if (!handle) return NeoLED::NEOLED_ERR_PARAM;
    return as_strip(handle)->begin(gpio, led_count, i2s_port, as_order(order), rgbw);
}

int neoled_strip_end(neoled_strip_handle_t handle)
{
    if (!handle) return NeoLED::NEOLED_ERR_PARAM;
    return as_strip(handle)->end();
}

int neoled_strip_set_pixel(neoled_strip_handle_t handle, uint16_t index,
                           uint8_t r, uint8_t g, uint8_t b)
{
    if (!handle) return NeoLED::NEOLED_ERR_PARAM;
    Pixel p = NeoLED::makePixel(r, g, b);
    return as_strip(handle)->setPixel(index, p);
}

int neoled_strip_set_pixel_rgbw(neoled_strip_handle_t handle, uint16_t index,
                                uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
    if (!handle) return NeoLED::NEOLED_ERR_PARAM;
    PixelW p;
    p.red = r; p.green = g; p.blue = b; p.white = w;
    return as_strip(handle)->setPixelW(index, p);
}

void neoled_strip_fill(neoled_strip_handle_t handle, uint8_t r, uint8_t g, uint8_t b)
{
    if (!handle) return;
    Pixel p = NeoLED::makePixel(r, g, b);
    as_strip(handle)->fill(p);
}

int neoled_strip_show(neoled_strip_handle_t handle)
{
    if (!handle) return NeoLED::NEOLED_ERR_PARAM;
    return as_strip(handle)->show();
}

int neoled_strip_clear(neoled_strip_handle_t handle)
{
    if (!handle) return NeoLED::NEOLED_ERR_PARAM;
    return as_strip(handle)->clear();
}

void neoled_strip_set_brightness(neoled_strip_handle_t handle, uint8_t brightness)
{
    if (handle) as_strip(handle)->setBrightness(brightness);
}

uint8_t neoled_strip_get_brightness(neoled_strip_handle_t handle)
{
    return handle ? as_strip(handle)->getBrightness() : 0;
}

uint16_t neoled_strip_num_leds(neoled_strip_handle_t handle)
{
    return handle ? as_strip(handle)->numLeds() : 0;
}

bool neoled_strip_is_initialized(neoled_strip_handle_t handle)
{
    return handle ? as_strip(handle)->isInitialized() : false;
}

const char* neoled_version(void)
{
    return NeoLED::version();
}

} // extern "C"
