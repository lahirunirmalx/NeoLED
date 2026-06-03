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

#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_rom_sys.h"   // esp_rom_delay_us()
#include "driver/gpio.h"
#include "neoled.h"

// ESP-IDF version-specific includes
#if NEOLED_USE_NEW_I2S_DRIVER
    // ESP-IDF 5.x uses new I2S driver
    #include "driver/i2s_std.h"
    #include "soc/soc_caps.h"
#else
    // ESP-IDF 4.x uses legacy I2S driver
    #include "driver/i2s.h"
#endif

static const char* TAG = "NeoLED";

namespace NeoLED {

// ============================================================================
// Shared constants
// ============================================================================

// Bit patterns for WS2812 timing via I2S (2 LED bits per byte).
static const uint16_t bitpatterns[4] = {0x88, 0x8e, 0xe8, 0xee};

// Reset/latch buffer (all zeros), shared read-only across instances.
static const uint8_t off_buffer[ZERO_BUFFER] = {0};

// WS2812 reset/latch time. The datasheet minimum is ~50 us; >= 280 us is the
// safe figure used by most drivers and works for WS2813/SK6812 too.
static const uint32_t LATCH_US = 280;

// Number of usable I2S ports on this SoC.
#if NEOLED_USE_NEW_I2S_DRIVER
    #define NEOLED_I2S_PORT_COUNT SOC_I2S_NUM
#else
    #define NEOLED_I2S_PORT_COUNT I2S_NUM_MAX
#endif

// Upper bound for the per-DMA-buffer length the I2S driver will accept.
static const uint32_t NEOLED_DMA_LEN_MAX = 1020;

// ============================================================================
// Internal Helpers
// ============================================================================

/**
 * @brief Encode one 8-bit color value into 4 I2S bytes (2 LED bits each).
 */
static inline void encodeByte(uint8_t value, uint8_t* dst)
{
    dst[0] = bitpatterns[(value >> 6) & 0x03];
    dst[1] = bitpatterns[(value >> 4) & 0x03];
    dst[2] = bitpatterns[(value >> 2) & 0x03];
    dst[3] = bitpatterns[value & 0x03];
}

static inline uint8_t scale8(uint8_t value, uint8_t brightness)
{
    return (uint8_t)(((uint16_t)value * brightness) / 255);
}

/**
 * @brief Arrange brightness-scaled r/g/b into wire order, append white if RGBW.
 * @return number of channel bytes written to wire[].
 */
static inline uint8_t toWireOrder(uint8_t r, uint8_t g, uint8_t b, uint8_t w,
                                  ColorOrder order, bool rgbw, uint8_t* wire)
{
    switch (order) {
        case ORDER_RGB: wire[0] = r; wire[1] = g; wire[2] = b; break;
        case ORDER_BRG: wire[0] = b; wire[1] = r; wire[2] = g; break;
        case ORDER_RBG: wire[0] = r; wire[1] = b; wire[2] = g; break;
        case ORDER_GBR: wire[0] = g; wire[1] = b; wire[2] = r; break;
        case ORDER_BGR: wire[0] = b; wire[1] = g; wire[2] = r; break;
        case ORDER_GRB:
        default:        wire[0] = g; wire[1] = r; wire[2] = b; break;
    }
    if (rgbw) {
        wire[3] = w;  // white is always last
        return 4;
    }
    return 3;
}

namespace {
// RAII lock guard around an (opaque) FreeRTOS mutex. A null handle is a no-op,
// so methods called before begin() simply skip locking.
struct LockGuard {
    SemaphoreHandle_t m;
    explicit LockGuard(void* mtx) : m(static_cast<SemaphoreHandle_t>(mtx))
    {
        if (m) xSemaphoreTake(m, portMAX_DELAY);
    }
    ~LockGuard()
    {
        if (m) xSemaphoreGive(m);
    }
    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;
};
} // anonymous namespace

// ============================================================================
// Strip — construction / teardown
// ============================================================================

Strip::Strip()
    : port_(I2S_NUM),
      gpio_(I2S_DO_IO),
      led_count_(0),
      size_buffer_(0),
      channels_(3),
      order_(ORDER_GRB),
      logical_(nullptr),
      out_buffer_(nullptr),
      brightness_(255),
      initialized_(false),
      mutex_(nullptr),
      tx_handle_(nullptr)
{
}

Strip::~Strip()
{
    end();
    if (mutex_) {
        vSemaphoreDelete(static_cast<SemaphoreHandle_t>(mutex_));
        mutex_ = nullptr;
    }
}

neoled_err_t Strip::begin(int gpio_pin, uint16_t led_count, int i2s_port,
                          ColorOrder order, bool rgbw)
{
    if (!mutex_) {
        mutex_ = xSemaphoreCreateMutex();
        if (!mutex_) {
            ESP_LOGE(TAG, "Failed to create mutex");
            return NEOLED_ERR_NO_MEM;
        }
    }

    LockGuard lg(mutex_);

    if (initialized_) {
        ESP_LOGW(TAG, "Already initialized, call end() first");
        return NEOLED_OK;
    }
    if (led_count == 0) {
        ESP_LOGE(TAG, "led_count must be >= 1");
        return NEOLED_ERR_PARAM;
    }
    if (i2s_port < 0 || i2s_port >= NEOLED_I2S_PORT_COUNT) {
        ESP_LOGE(TAG, "Invalid I2S port %d (this SoC has %d)", i2s_port, (int)NEOLED_I2S_PORT_COUNT);
        return NEOLED_ERR_PARAM;
    }

    port_        = i2s_port;
    gpio_        = gpio_pin;
    led_count_   = led_count;
    order_       = order;
    channels_    = rgbw ? 4 : 3;
    size_buffer_ = (uint16_t)(led_count * channels_ * 4);

    logical_    = (uint8_t*)calloc(led_count, channels_);
    out_buffer_ = (uint8_t*)malloc(size_buffer_);
    if (!logical_ || !out_buffer_) {
        ESP_LOGE(TAG, "Failed to allocate strip buffers");
        free(logical_);    logical_ = nullptr;
        free(out_buffer_); out_buffer_ = nullptr;
        return NEOLED_ERR_NO_MEM;
    }

    uint32_t dma_len = size_buffer_;
    if (dma_len > NEOLED_DMA_LEN_MAX) dma_len = NEOLED_DMA_LEN_MAX;

    esp_err_t ret;

#if NEOLED_USE_NEW_I2S_DRIVER
    i2s_chan_config_t chan_cfg = {
        .id = (i2s_port_t)port_,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 4,
        .dma_frame_num = dma_len,
        .auto_clear = true
    };

    i2s_chan_handle_t handle = NULL;
    ret = i2s_new_channel(&chan_cfg, &handle, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2S channel: %s", esp_err_to_name(ret));
        free(logical_); logical_ = nullptr;
        free(out_buffer_); out_buffer_ = nullptr;
        return NEOLED_ERR_I2S;
    }

    // Use the official default-config macros for clk_cfg and slot_cfg. They
    // expand to exactly the Philips-standard 16-bit stereo settings this driver
    // needs, and crucially they handle the SoC-dependent slot fields
    // (msb_right on I2S HW v1 / ESP32-S2 vs left_align/big_endian/bit_order_lsb
    // on HW v2) which differ between targets.
    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                        I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_GPIO_UNUSED,
            .ws = I2S_GPIO_UNUSED,
            .dout = (gpio_num_t)gpio_pin,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false
            }
        }
    };

    ret = i2s_channel_init_std_mode(handle, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init I2S channel: %s", esp_err_to_name(ret));
        i2s_del_channel(handle);
        free(logical_); logical_ = nullptr;
        free(out_buffer_); out_buffer_ = nullptr;
        return NEOLED_ERR_I2S;
    }

    ret = i2s_channel_enable(handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S channel: %s", esp_err_to_name(ret));
        i2s_del_channel(handle);
        free(logical_); logical_ = nullptr;
        free(out_buffer_); out_buffer_ = nullptr;
        return NEOLED_ERR_I2S;
    }

    tx_handle_ = handle;
#else
    i2s_config_t i2s_config = {
        .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
        .communication_format = static_cast<i2s_comm_format_t>(I2S_COMM_FORMAT_STAND_I2S | I2S_COMM_FORMAT_STAND_MSB),
    #else
        .communication_format = static_cast<i2s_comm_format_t>(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    #endif
        .intr_alloc_flags = 0,
        .dma_buf_count = 4,
        .dma_buf_len = (int)dma_len,
        .use_apll = false,
        // Field order must match the struct declaration for C++ designated
        // initializers: tx_desc_auto_clear precedes mclk_multiple in 4.x.
        .tx_desc_auto_clear = true,
    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
        .mclk_multiple = I2S_MCLK_MULTIPLE_DEFAULT
    #endif
    };

    i2s_pin_config_t pin_config = {
    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
        .mck_io_num = I2S_PIN_NO_CHANGE,
    #endif
        .bck_io_num = I2S_PIN_NO_CHANGE,
        .ws_io_num = I2S_PIN_NO_CHANGE,
        .data_out_num = gpio_pin,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    ret = i2s_driver_install(static_cast<i2s_port_t>(port_), &i2s_config, 0, nullptr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2S driver: %s", esp_err_to_name(ret));
        free(logical_); logical_ = nullptr;
        free(out_buffer_); out_buffer_ = nullptr;
        return NEOLED_ERR_I2S;
    }

    ret = i2s_set_pin(static_cast<i2s_port_t>(port_), &pin_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set I2S pins: %s", esp_err_to_name(ret));
        i2s_driver_uninstall(static_cast<i2s_port_t>(port_));
        free(logical_); logical_ = nullptr;
        free(out_buffer_); out_buffer_ = nullptr;
        return NEOLED_ERR_I2S;
    }
#endif

    initialized_ = true;
    ESP_LOGI(TAG, "Initialized I2S%d on GPIO %d, %u LEDs (%s, order %d)",
             port_, gpio_pin, (unsigned)led_count_, channels_ == 4 ? "RGBW" : "RGB", (int)order_);

    // Clear LEDs on init (all-zero pixel encodes to bitpatterns[0]).
    memset(out_buffer_, bitpatterns[0], size_buffer_);
    transmit();

    return NEOLED_OK;
}

neoled_err_t Strip::end(void)
{
    LockGuard lg(mutex_);

    if (!initialized_) {
        return NEOLED_OK;
    }

    memset(out_buffer_, bitpatterns[0], size_buffer_);
    transmit();

    esp_err_t ret;

#if NEOLED_USE_NEW_I2S_DRIVER
    if (tx_handle_) {
        i2s_chan_handle_t handle = static_cast<i2s_chan_handle_t>(tx_handle_);
        ret = i2s_channel_disable(handle);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to disable I2S channel: %s", esp_err_to_name(ret));
        }
        ret = i2s_del_channel(handle);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to delete I2S channel: %s", esp_err_to_name(ret));
        }
        tx_handle_ = nullptr;
    }
#else
    ret = i2s_driver_uninstall(static_cast<i2s_port_t>(port_));
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to uninstall I2S driver: %s", esp_err_to_name(ret));
    }
#endif

    gpio_reset_pin(static_cast<gpio_num_t>(gpio_));

    free(logical_);    logical_ = nullptr;
    free(out_buffer_); out_buffer_ = nullptr;
    size_buffer_ = 0;
    initialized_ = false;
    ESP_LOGI(TAG, "Strip on I2S%d destroyed", port_);

    return NEOLED_OK;
}

// ============================================================================
// Strip — rendering pipeline
// ============================================================================

void Strip::copyLogical(const Pixel* pixels)
{
    for (uint16_t i = 0; i < led_count_; i++) {
        uint8_t* dst = &logical_[i * channels_];
        dst[0] = pixels[i].red;
        dst[1] = pixels[i].green;
        dst[2] = pixels[i].blue;
        if (channels_ == 4) dst[3] = 0;  // update(Pixel*) leaves white off
    }
}

void Strip::render(uint8_t brightness)
{
    uint8_t wire[4];
    bool rgbw = (channels_ == 4);
    for (uint16_t i = 0; i < led_count_; i++) {
        const uint8_t* src = &logical_[i * channels_];
        uint8_t r = scale8(src[0], brightness);
        uint8_t g = scale8(src[1], brightness);
        uint8_t b = scale8(src[2], brightness);
        uint8_t w = rgbw ? scale8(src[3], brightness) : 0;

        uint8_t n = toWireOrder(r, g, b, w, order_, rgbw, wire);
        uint8_t* dst = &out_buffer_[i * channels_ * 4];
        for (uint8_t c = 0; c < n; c++) {
            encodeByte(wire[c], &dst[c * 4]);
        }
    }
}

neoled_err_t Strip::writeData(void)
{
    size_t bytes_written = 0;
    esp_err_t ret;
#if NEOLED_USE_NEW_I2S_DRIVER
    ret = i2s_channel_write(static_cast<i2s_chan_handle_t>(tx_handle_),
                            out_buffer_, size_buffer_, &bytes_written, portMAX_DELAY);
#else
    ret = i2s_write(static_cast<i2s_port_t>(port_),
                    out_buffer_, size_buffer_, &bytes_written, portMAX_DELAY);
#endif
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S write failed: %s", esp_err_to_name(ret));
        return NEOLED_ERR_I2S;
    }
    return NEOLED_OK;
}

neoled_err_t Strip::writeReset(void)
{
    size_t bytes_written = 0;
    esp_err_t ret;
#if NEOLED_USE_NEW_I2S_DRIVER
    ret = i2s_channel_write(static_cast<i2s_chan_handle_t>(tx_handle_),
                            off_buffer, ZERO_BUFFER, &bytes_written, portMAX_DELAY);
#else
    ret = i2s_write(static_cast<i2s_port_t>(port_),
                    off_buffer, ZERO_BUFFER, &bytes_written, portMAX_DELAY);
#endif
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S write (reset) failed: %s", esp_err_to_name(ret));
        return NEOLED_ERR_I2S;
    }
    return NEOLED_OK;
}

void Strip::zeroDma(void)
{
#if !NEOLED_USE_NEW_I2S_DRIVER
    i2s_zero_dma_buffer(static_cast<i2s_port_t>(port_));
#endif
}

neoled_err_t Strip::transmit(void)
{
    neoled_err_t e = writeData();
    if (e != NEOLED_OK) return e;
    e = writeReset();
    if (e != NEOLED_OK) return e;
    esp_rom_delay_us(LATCH_US);  // deterministic WS2812 reset/latch
    zeroDma();
    return NEOLED_OK;
}

// ============================================================================
// Strip — whole-frame updates
// ============================================================================

neoled_err_t Strip::update(const Pixel* pixels)
{
    return updateWithBrightness(pixels, brightness_);
}

neoled_err_t Strip::updateWithBrightness(const Pixel* pixels, uint8_t brightness)
{
    LockGuard lg(mutex_);
    if (!initialized_) { ESP_LOGE(TAG, "Not initialized"); return NEOLED_ERR_NOT_INIT; }
    if (pixels == nullptr) { ESP_LOGE(TAG, "Null pixel pointer"); return NEOLED_ERR_PARAM; }

    copyLogical(pixels);
    render(brightness);
    return transmit();
}

neoled_err_t Strip::updateW(const PixelW* pixels)
{
    LockGuard lg(mutex_);
    if (!initialized_) { ESP_LOGE(TAG, "Not initialized"); return NEOLED_ERR_NOT_INIT; }
    if (pixels == nullptr) { ESP_LOGE(TAG, "Null pixel pointer"); return NEOLED_ERR_PARAM; }

    for (uint16_t i = 0; i < led_count_; i++) {
        uint8_t* dst = &logical_[i * channels_];
        dst[0] = pixels[i].red;
        dst[1] = pixels[i].green;
        dst[2] = pixels[i].blue;
        if (channels_ == 4) dst[3] = pixels[i].white;
    }
    render(brightness_);
    return transmit();
}

neoled_err_t Strip::clear(void)
{
    LockGuard lg(mutex_);
    if (!initialized_) return NEOLED_ERR_NOT_INIT;

    memset(logical_, 0, (size_t)led_count_ * channels_);
    memset(out_buffer_, bitpatterns[0], size_buffer_);  // all-zero pixel
    return transmit();
}

// ============================================================================
// Strip — per-pixel framebuffer API
// ============================================================================

neoled_err_t Strip::setPixel(uint16_t index, const Pixel& color)
{
    if (!initialized_ || index >= led_count_) return NEOLED_ERR_PARAM;
    uint8_t* dst = &logical_[index * channels_];
    dst[0] = color.red;
    dst[1] = color.green;
    dst[2] = color.blue;
    return NEOLED_OK;
}

neoled_err_t Strip::setPixelW(uint16_t index, const PixelW& color)
{
    if (!initialized_ || index >= led_count_) return NEOLED_ERR_PARAM;
    uint8_t* dst = &logical_[index * channels_];
    dst[0] = color.red;
    dst[1] = color.green;
    dst[2] = color.blue;
    if (channels_ == 4) dst[3] = color.white;
    return NEOLED_OK;
}

Pixel Strip::getPixel(uint16_t index) const
{
    Pixel p = {0, 0, 0};
    if (!initialized_ || index >= led_count_) return p;
    const uint8_t* src = &logical_[index * channels_];
    p.red = src[0];
    p.green = src[1];
    p.blue = src[2];
    return p;
}

void Strip::fill(const Pixel& color)
{
    if (!initialized_) return;
    for (uint16_t i = 0; i < led_count_; i++) {
        uint8_t* dst = &logical_[i * channels_];
        dst[0] = color.red;
        dst[1] = color.green;
        dst[2] = color.blue;
    }
}

neoled_err_t Strip::fillRange(uint16_t start, uint16_t count, const Pixel& color)
{
    if (!initialized_ || start >= led_count_) return NEOLED_ERR_PARAM;
    uint16_t end = start + count;
    if (end > led_count_) end = led_count_;
    for (uint16_t i = start; i < end; i++) {
        uint8_t* dst = &logical_[i * channels_];
        dst[0] = color.red;
        dst[1] = color.green;
        dst[2] = color.blue;
    }
    return NEOLED_OK;
}

neoled_err_t Strip::show(void)
{
    LockGuard lg(mutex_);
    if (!initialized_) return NEOLED_ERR_NOT_INIT;
    render(brightness_);
    return transmit();
}

// ============================================================================
// Strip — accessors
// ============================================================================

bool       Strip::isInitialized(void) const { return initialized_; }
void       Strip::setBrightness(uint8_t b)   { brightness_ = b; }
uint8_t    Strip::getBrightness(void) const  { return brightness_; }
uint16_t   Strip::numLeds(void) const        { return led_count_; }
int        Strip::getGpioPin(void) const     { return gpio_; }
int        Strip::getPort(void) const        { return port_; }
ColorOrder Strip::getColorOrder(void) const  { return order_; }
bool       Strip::isRGBW(void) const         { return channels_ == 4; }

// ============================================================================
// Parallel multi-strip update
// ============================================================================

neoled_err_t updateParallel(Strip* const* strips, const Pixel* const* pixels, uint8_t count)
{
    if (!strips || !pixels || count == 0) {
        return NEOLED_ERR_PARAM;
    }

    for (uint8_t i = 0; i < count; i++) {
        if (!strips[i]) return NEOLED_ERR_PARAM;
        if (strips[i]->mutex_) {
            xSemaphoreTake(static_cast<SemaphoreHandle_t>(strips[i]->mutex_), portMAX_DELAY);
        }
    }

    neoled_err_t result = NEOLED_OK;

    // Phase 1: render every buffer (pure CPU work).
    for (uint8_t i = 0; i < count; i++) {
        if (!strips[i]->initialized_) { result = NEOLED_ERR_NOT_INIT; break; }
        if (!pixels[i])               { result = NEOLED_ERR_PARAM;    break; }
        strips[i]->copyLogical(pixels[i]);
        strips[i]->render(strips[i]->brightness_);
    }

    // Phase 2: kick the DMA writes back-to-back so peripherals overlap.
    if (result == NEOLED_OK) {
        for (uint8_t i = 0; i < count; i++) {
            neoled_err_t e = strips[i]->writeData();
            if (e != NEOLED_OK) result = e;
        }
        for (uint8_t i = 0; i < count; i++) {
            neoled_err_t e = strips[i]->writeReset();
            if (e != NEOLED_OK) result = e;
        }
        esp_rom_delay_us(LATCH_US);  // one latch covers all (ran in parallel)
        for (uint8_t i = 0; i < count; i++) {
            strips[i]->zeroDma();
        }
    }

    for (int i = (int)count - 1; i >= 0; i--) {
        if (strips[i]->mutex_) {
            xSemaphoreGive(static_cast<SemaphoreHandle_t>(strips[i]->mutex_));
        }
    }

    return result;
}

// ============================================================================
// Default strip + backward-compatible free functions
// ============================================================================

static Strip g_default;

neoled_err_t init(void)
{
    return g_default.begin(I2S_DO_IO, LED_NUMBER, I2S_NUM);
}

neoled_err_t initWithPin(int gpio_pin)
{
    return g_default.begin(gpio_pin, LED_NUMBER, I2S_NUM);
}

neoled_err_t update(const Pixel* pixels)
{
    return g_default.update(pixels);
}

neoled_err_t updateWithBrightness(const Pixel* pixels, uint8_t brightness)
{
    return g_default.updateWithBrightness(pixels, brightness);
}

neoled_err_t clear(void)
{
    return g_default.clear();
}

neoled_err_t destroy(void)
{
    return g_default.end();
}

bool isInitialized(void)
{
    return g_default.isInitialized();
}

void setBrightness(uint8_t brightness)
{
    g_default.setBrightness(brightness);
}

uint8_t getBrightness(void)
{
    return g_default.getBrightness();
}

uint16_t numLeds(void)
{
    // Reports the compile-time build size for backward compatibility.
    return LED_NUMBER;
}

int getGpioPin(void)
{
    return g_default.getGpioPin();
}

const char* version(void)
{
    return NEOLED_VERSION_STRING;
}

// ============================================================================
// Gamma Correction
// ============================================================================

// Gamma correction lookup table (gamma = 2.2)
static const uint8_t gamma22_table[256] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,
    2,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   5,   5,   5,
    5,   6,   6,   6,   6,   7,   7,   7,   7,   8,   8,   8,   9,   9,   9,  10,
   10,  10,  11,  11,  11,  12,  12,  13,  13,  13,  14,  14,  15,  15,  16,  16,
   17,  17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  22,  23,  24,  24,  25,
   25,  26,  27,  27,  28,  29,  29,  30,  31,  32,  32,  33,  34,  35,  35,  36,
   37,  38,  39,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  50,
   51,  52,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  66,  67,  68,
   69,  70,  72,  73,  74,  75,  77,  78,  79,  81,  82,  83,  85,  86,  87,  89,
   90,  92,  93,  95,  96,  98,  99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};

Pixel gammaCorrect(const Pixel& pixel, float gamma)
{
    Pixel result;

    if (gamma == 2.2f) {
        result.red = gamma22_table[pixel.red];
        result.green = gamma22_table[pixel.green];
        result.blue = gamma22_table[pixel.blue];
    } else {
        result.red = (uint8_t)(powf(pixel.red / 255.0f, gamma) * 255.0f + 0.5f);
        result.green = (uint8_t)(powf(pixel.green / 255.0f, gamma) * 255.0f + 0.5f);
        result.blue = (uint8_t)(powf(pixel.blue / 255.0f, gamma) * 255.0f + 0.5f);
    }

    return result;
}

} // namespace NeoLED
