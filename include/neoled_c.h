/** MIT licence — see LICENSE.

 NeoLED C API.

 A thin C wrapper around the C++ NeoLED::Strip class, so plain-C ESP-IDF
 projects (the majority) can use the library without writing C++. Each handle
 wraps one independent strip on one I2S channel.

 All int-returning functions return 0 (NEOLED_OK) on success or a negative
 neoled_err_t code on failure (see neoled.h: -2 PARAM, -3 NO_MEM, -4 NOT_INIT,
 -5 I2S).
*/
#ifndef NEOLED_C_H
#define NEOLED_C_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque handle to a single NeoLED strip. */
typedef struct neoled_strip* neoled_strip_handle_t;

/** Wire color order (mirrors NeoLED::ColorOrder). */
typedef enum {
    NEOLED_ORDER_GRB = 0,  /* WS2812/SK6812 (default) */
    NEOLED_ORDER_RGB,      /* WS2811/PL9823/APA106    */
    NEOLED_ORDER_BRG,
    NEOLED_ORDER_RBG,
    NEOLED_ORDER_GBR,
    NEOLED_ORDER_BGR
} neoled_color_order_t;

/** @brief Allocate a strip object. Returns NULL on allocation failure. */
neoled_strip_handle_t neoled_strip_create(void);

/** @brief Free a strip object (calls end() first if still initialized). */
void neoled_strip_destroy(neoled_strip_handle_t handle);

/** @brief Initialize a GRB RGB strip. @return 0 on success. */
int neoled_strip_begin(neoled_strip_handle_t handle, int gpio, uint16_t led_count, int i2s_port);

/** @brief Initialize with explicit color order and RGB/RGBW. @return 0 on success. */
int neoled_strip_begin_ex(neoled_strip_handle_t handle, int gpio, uint16_t led_count,
                          int i2s_port, neoled_color_order_t order, bool rgbw);

/** @brief Release the I2S channel and buffers. @return 0 on success. */
int neoled_strip_end(neoled_strip_handle_t handle);

/** @brief Set one RGB pixel in the framebuffer (no transmit). @return 0 on success. */
int neoled_strip_set_pixel(neoled_strip_handle_t handle, uint16_t index,
                           uint8_t r, uint8_t g, uint8_t b);

/** @brief Set one RGBW pixel in the framebuffer (no transmit). @return 0 on success. */
int neoled_strip_set_pixel_rgbw(neoled_strip_handle_t handle, uint16_t index,
                                uint8_t r, uint8_t g, uint8_t b, uint8_t w);

/** @brief Fill the whole framebuffer with one RGB color (no transmit). */
void neoled_strip_fill(neoled_strip_handle_t handle, uint8_t r, uint8_t g, uint8_t b);

/** @brief Render the framebuffer and transmit it. @return 0 on success. */
int neoled_strip_show(neoled_strip_handle_t handle);

/** @brief Turn all LEDs off and transmit. @return 0 on success. */
int neoled_strip_clear(neoled_strip_handle_t handle);

void     neoled_strip_set_brightness(neoled_strip_handle_t handle, uint8_t brightness);
uint8_t  neoled_strip_get_brightness(neoled_strip_handle_t handle);
uint16_t neoled_strip_num_leds(neoled_strip_handle_t handle);
bool     neoled_strip_is_initialized(neoled_strip_handle_t handle);

/** @brief Library version string, e.g. "1.4.0". */
const char* neoled_version(void);

#ifdef __cplusplus
}
#endif

#endif // NEOLED_C_H
