/* Host demo entry point: run a few capture cycles against the mock HAL.
 *
 * This is the dev-machine stand-in for the firmware main loop. On the ESP32 the
 * entry point becomes app_main() (ESP-IDF) calling the same device API.
 */
#include <stdio.h>

#include "device.h"
#include "sudonit/log.h"

int main(void) {
    sd_log_set_level(SD_LOG_DEBUG);

    if (sd_device_init() != SD_OK) {
        return 1;
    }

    for (int i = 0; i < 3; ++i) {
        sd_device_status_t status = {0};
        sd_err_t err = sd_device_capture_cycle(&status);
        if (err != SD_OK) {
            return 1;
        }
        printf("cycle %d: image=%zu bytes (%ux%u), battery=%u%%\n", i,
               status.image_bytes, status.image_width, status.image_height,
               status.battery_percent);
    }
    return 0;
}
