/* Mock battery backend (host build).
 *
 * Reports a fixed percentage, overridable via the SUDONIT_MOCK_BATTERY env var
 * so tests/diagnostics can script low-battery behaviour. On hardware,
 * battery_esp.c replaces this with an ADC read of the VBAT divider.
 */
#include "sudonit/hal/battery.h"

#include <stdlib.h>

#define DEFAULT_PERCENT 87

static uint8_t s_percent = DEFAULT_PERCENT;

sd_err_t sd_battery_init(void) {
    const char *override = getenv("SUDONIT_MOCK_BATTERY");
    if (override) {
        int v = atoi(override);
        if (v < 0) v = 0;
        if (v > 100) v = 100;
        s_percent = (uint8_t)v;
    }
    return SD_OK;
}

sd_err_t sd_battery_read(uint8_t *percent_out) {
    if (!percent_out) {
        return SD_ERR_INVALID;
    }
    *percent_out = s_percent;
    return SD_OK;
}

const char *sd_battery_backend(void) { return "mock"; }
