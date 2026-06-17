/* Sudonit HAL — battery.
 *
 * Reads state of charge as a percentage. Mock returns a scripted value on the
 * host; the real backend reads an ADC on the VBAT divider. Battery is product
 * priority #2 (MASTER.md), so this seam exists from day one.
 */
#ifndef SUDONIT_HAL_BATTERY_H
#define SUDONIT_HAL_BATTERY_H

#include <stdint.h>

#include "sudonit/error.h"

sd_err_t sd_battery_init(void);

/* On SD_OK, *percent_out is 0..100. */
sd_err_t sd_battery_read(uint8_t *percent_out);

const char *sd_battery_backend(void);

#endif /* SUDONIT_HAL_BATTERY_H */
