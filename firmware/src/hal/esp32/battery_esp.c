/* ESP32 battery backend — STUB (no hardware behavior).
 *
 * The real driver reads an ADC on the VBAT divider and calibrates to a
 * percentage. This stub does NOT invent a value: read returns
 * SD_ERR_UNSUPPORTED until the ADC driver lands. See
 * docs/HARDWARE_INTEGRATION_PLAN.md section 5.
 */
#include "sudonit/hal/battery.h"

sd_err_t sd_battery_init(void) {
    return SD_OK; /* no-op init; real init configures the ADC channel */
}

sd_err_t sd_battery_read(uint8_t *percent_out) {
    (void)percent_out;
    return SD_ERR_UNSUPPORTED; /* TODO: ADC read of VBAT divider */
}

const char *sd_battery_backend(void) { return "adc-vbat (stub)"; }
