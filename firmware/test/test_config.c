/* Host tests for the configuration subsystem (file backend).
 *
 * Uses SUDONIT_CONFIG_PATH to isolate state in a temp file so the suite is
 * repeatable and side-effect-free.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sudonit/config.h"

static int g_failures;

#define CHECK(cond, msg)                                                  \
    do {                                                                  \
        if (!(cond)) {                                                    \
            fprintf(stderr, "FAIL: %s (%s:%d)\n", (msg), __FILE__, __LINE__); \
            ++g_failures;                                                 \
        }                                                                 \
    } while (0)

static const char *TEST_PATH = "/tmp/sudonit_test_config.bin";

static void test_defaults(void) {
    sd_config_t c;
    sd_config_defaults(&c);
    CHECK(strcmp(c.device_name, "sudonit-glasses") == 0, "default device name");
    CHECK(c.server_port == 8765, "default port");
    CHECK(c.wifi_ssid[0] == '\0', "default ssid empty");
    CHECK(c.wifi_password[0] == '\0', "default password empty");
    CHECK(c.server_host[0] == '\0', "default host empty");
}

static void test_load_absent_gives_defaults(void) {
    unlink(TEST_PATH);
    sd_config_t c;
    CHECK(sd_config_load(&c) == SD_OK, "load absent -> SD_OK");
    CHECK(strcmp(c.device_name, "sudonit-glasses") == 0, "absent -> defaults");
}

static void test_save_load_roundtrip(void) {
    unlink(TEST_PATH);
    sd_config_t in;
    sd_config_defaults(&in);
    snprintf(in.wifi_ssid, sizeof(in.wifi_ssid), "MyNetwork");
    snprintf(in.wifi_password, sizeof(in.wifi_password), "s3cr3t-pass");
    snprintf(in.server_host, sizeof(in.server_host), "192.168.1.42");
    in.server_port = 9000;
    snprintf(in.device_name, sizeof(in.device_name), "alex-glasses");

    CHECK(sd_config_save(&in) == SD_OK, "save");

    sd_config_t out;
    CHECK(sd_config_load(&out) == SD_OK, "load");
    CHECK(strcmp(out.wifi_ssid, "MyNetwork") == 0, "ssid roundtrip");
    CHECK(strcmp(out.wifi_password, "s3cr3t-pass") == 0, "password roundtrip");
    CHECK(strcmp(out.server_host, "192.168.1.42") == 0, "host roundtrip");
    CHECK(out.server_port == 9000, "port roundtrip");
    CHECK(strcmp(out.device_name, "alex-glasses") == 0, "name roundtrip");
}

static void test_version_mismatch_gives_defaults(void) {
    /* Write a blob with a bogus version header; load must fall back. */
    FILE *f = fopen(TEST_PATH, "wb");
    CHECK(f != NULL, "open temp for bogus write");
    if (f) {
        /* version header + a correctly-sized config body, but a bad version, so
         * load() hits the version-mismatch path (not the too-small path). */
        uint32_t bad_version = 999;
        char body[sizeof(sd_config_t)] = {0};
        fwrite(&bad_version, sizeof(bad_version), 1, f);
        fwrite(body, 1, sizeof(body), f);
        fclose(f);
    }
    sd_config_t c;
    CHECK(sd_config_load(&c) == SD_OK, "load bogus -> SD_OK");
    CHECK(strcmp(c.device_name, "sudonit-glasses") == 0, "bogus version -> defaults");
}

int main(void) {
    setenv("SUDONIT_CONFIG_PATH", TEST_PATH, 1);

    test_defaults();
    test_load_absent_gives_defaults();
    test_save_load_roundtrip();
    test_version_mismatch_gives_defaults();

    unlink(TEST_PATH);

    if (g_failures == 0) {
        printf("all config tests passed\n");
        return 0;
    }
    fprintf(stderr, "%d config test(s) failed\n", g_failures);
    return 1;
}
