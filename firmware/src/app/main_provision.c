/* device_provision — host driver for the serial provisioning console.
 *
 * Reads commands from stdin and writes to stdout, exactly as the on-device
 * console reads/writes the UART. This is the hardware-free way to exercise the
 * provisioning flow; on the ESP32 the same sd_provision_repl() runs over UART.
 *
 *   echo "set server_host 127.0.0.1" | device_provision
 *   device_provision        # interactive
 *
 * Config persists through the host backend (SUDONIT_CONFIG_PATH).
 */
#include <stdio.h>

#include "sudonit/provisioning.h"

int main(void) {
    sd_provision_repl(stdin, stdout);
    return 0;
}
