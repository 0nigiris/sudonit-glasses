/* Sudonit firmware — serial provisioning console.
 *
 * A transport-independent command processor for provisioning and recovery. The
 * command logic (handle_line) is decoupled from any I/O so it can be unit-tested
 * by feeding it a string and capturing output via a sink callback. The REPL
 * helper drives it over a FILE* stream — stdin/stdout on the host, the UART
 * console on the ESP32 (ESP-IDF maps stdio to UART).
 *
 * Commands: help | show | get <key> | set <key> <value> | save | reset
 * Keys: device_name | wifi_ssid | wifi_password | server_host | server_port
 *
 * The Wi-Fi password is never echoed (show/get report set/unset).
 */
#ifndef SUDONIT_PROVISIONING_H
#define SUDONIT_PROVISIONING_H

#include <stdio.h>

#include "sudonit/config.h"
#include "sudonit/error.h"

/* Output sink: called once per output line (no trailing newline in `line`). */
typedef void (*sd_provision_out_fn)(void *ctx, const char *line);

typedef struct {
    sd_config_t cfg;
    int dirty; /* unsaved changes since load/save */
} sd_provision_session_t;

/* Load current config into a fresh session. */
void sd_provision_init(sd_provision_session_t *s);

/* Handle one command line. Output is emitted via `out`. Returns SD_OK once the
 * line is processed (command errors are reported through `out`, not the return
 * value); SD_ERR_INVALID only for a NULL argument. */
sd_err_t sd_provision_handle_line(sd_provision_session_t *s, const char *line,
                                  sd_provision_out_fn out, void *ctx);

/* Blocking REPL over a stream pair until EOF. Transport-independent: pass
 * stdin/stdout on the host, or the UART console FILE*s on the device. */
void sd_provision_repl(FILE *in, FILE *out);

#endif /* SUDONIT_PROVISIONING_H */
