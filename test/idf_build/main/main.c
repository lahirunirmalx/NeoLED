/* CI build/link smoke test for the NeoLED C API (plain C translation unit). */
#include <stdio.h>
#include "neoled_c.h"

void app_main(void)
{
    printf("NeoLED %s\n", neoled_version());

    neoled_strip_handle_t strip = neoled_strip_create();
    if (neoled_strip_begin_ex(strip, 21, 8, 0, NEOLED_ORDER_GRB, false) == 0) {
        neoled_strip_set_brightness(strip, 64);
        neoled_strip_fill(strip, 0, 80, 0);
        neoled_strip_set_pixel(strip, 0, 255, 0, 0);
        neoled_strip_show(strip);
        neoled_strip_clear(strip);
        neoled_strip_end(strip);
    }
    neoled_strip_destroy(strip);
}
