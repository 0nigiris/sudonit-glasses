/* device_config — host CLI to read/write device configuration.
 *
 * A thin wrapper over the shared config API, persisting through the host file
 * backend (SUDONIT_CONFIG_PATH, default ./sudonit_config.bin). It's how a
 * developer provisions the host firmware today; the same config the ESP32 reads
 * from NVS. The on-device provisioning flow is designed in
 * docs/PROVISIONING_PLAN.md.
 *
 *   device_config show
 *   device_config get <key>
 *   device_config set <key> <value>
 *
 * keys: device_name | wifi_ssid | wifi_password | server_host | server_port
 *
 * The Wi-Fi password is never printed in plaintext (show/get report set/unset).
 */
#include <stdio.h>
#include <string.h>

#include "sudonit/config.h"

static void usage(void) {
    fprintf(stderr,
            "usage:\n"
            "  device_config show\n"
            "  device_config get <key>\n"
            "  device_config set <key> <value>\n"
            "keys: device_name wifi_ssid wifi_password server_host server_port\n");
}

static void print_show(const sd_config_t *cfg) {
    printf("device_name   = %s\n", cfg->device_name);
    printf("wifi_ssid     = %s\n", cfg->wifi_ssid[0] ? cfg->wifi_ssid : "(unset)");
    printf("wifi_password = %s\n", cfg->wifi_password[0] ? "(set)" : "(unset)");
    printf("server_host   = %s\n", cfg->server_host[0] ? cfg->server_host : "(unset)");
    printf("server_port   = %u\n", (unsigned)cfg->server_port);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage();
        return 2;
    }

    sd_config_t cfg;
    sd_config_load(&cfg);
    const char *cmd = argv[1];

    if (strcmp(cmd, "show") == 0) {
        print_show(&cfg);
        return 0;
    }

    if (strcmp(cmd, "get") == 0) {
        if (argc < 3) {
            usage();
            return 2;
        }
        const char *key = argv[2];
        /* Never reveal the password in plaintext. */
        if (strcmp(key, "wifi_password") == 0) {
            printf("%s\n", cfg.wifi_password[0] ? "(set)" : "(unset)");
            return 0;
        }
        char value[128];
        if (sd_config_get_field(&cfg, key, value, sizeof(value)) != SD_OK) {
            fprintf(stderr, "unknown key: %s\n", key);
            return 2;
        }
        printf("%s\n", value);
        return 0;
    }

    if (strcmp(cmd, "set") == 0) {
        if (argc < 4) {
            usage();
            return 2;
        }
        if (sd_config_set_field(&cfg, argv[2], argv[3]) != SD_OK) {
            fprintf(stderr, "invalid key or value: %s = %s\n", argv[2], argv[3]);
            return 2;
        }
        if (sd_config_save(&cfg) != SD_OK) {
            fprintf(stderr, "failed to save configuration\n");
            return 1;
        }
        /* Confirm without echoing a secret value. */
        if (strcmp(argv[2], "wifi_password") == 0) {
            printf("set wifi_password = (hidden)\n");
        } else {
            printf("set %s = %s\n", argv[2], argv[3]);
        }
        return 0;
    }

    usage();
    return 2;
}
