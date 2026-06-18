/* Host tests for the serial provisioning command processor.
 *
 * Drives sd_provision_handle_line with strings and captures output via a sink,
 * so the whole flow is exercised with no UART/hardware. Config persists to a
 * temp file (SUDONIT_CONFIG_PATH).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sudonit/config.h"
#include "sudonit/provisioning.h"

static int g_failures;

#define CHECK(cond, msg)                                                  \
    do {                                                                  \
        if (!(cond)) {                                                    \
            fprintf(stderr, "FAIL: %s (%s:%d)\n", (msg), __FILE__, __LINE__); \
            ++g_failures;                                                 \
        }                                                                 \
    } while (0)

static const char *TEST_PATH = "/tmp/sudonit_test_prov.bin";

/* Capturing sink: accumulate emitted lines into one buffer, newline-joined. */
struct capture {
    char buf[2048];
    size_t len;
};

static void cap_sink(void *ctx, const char *line) {
    struct capture *c = ctx;
    int n = snprintf(c->buf + c->len, sizeof(c->buf) - c->len, "%s\n", line);
    if (n > 0) {
        c->len += (size_t)n;
    }
}

/* Run one command line against the session, return captured output. */
static void run(sd_provision_session_t *s, const char *line, struct capture *c) {
    c->buf[0] = '\0';
    c->len = 0;
    sd_provision_handle_line(s, line, cap_sink, c);
}

static void test_help_lists_commands(void) {
    sd_provision_session_t s;
    sd_provision_init(&s);
    struct capture c;
    run(&s, "help", &c);
    CHECK(strstr(c.buf, "show") && strstr(c.buf, "set") && strstr(c.buf, "save") &&
              strstr(c.buf, "reset"),
          "help lists commands");
}

static void test_set_get_show_mask(void) {
    sd_provision_session_t s;
    sd_provision_init(&s);
    struct capture c;

    run(&s, "set server_host 10.1.2.3", &c);
    CHECK(strstr(c.buf, "ok:") != NULL, "set host ok");
    CHECK(s.dirty == 1, "set marks dirty");

    run(&s, "get server_host", &c);
    CHECK(strcmp(c.buf, "10.1.2.3\n") == 0, "get host value");

    /* Password must never be echoed. */
    run(&s, "set wifi_password hunter2", &c);
    CHECK(strstr(c.buf, "hunter2") == NULL, "set does not echo password");
    run(&s, "get wifi_password", &c);
    CHECK(strstr(c.buf, "hunter2") == NULL && strstr(c.buf, "(set)") != NULL,
          "get masks password");
    run(&s, "show", &c);
    CHECK(strstr(c.buf, "hunter2") == NULL && strstr(c.buf, "wifi_password = (set)"),
          "show masks password");
}

static void test_password_with_spaces(void) {
    sd_provision_session_t s;
    sd_provision_init(&s);
    struct capture c;
    run(&s, "set wifi_password pass with spaces", &c);
    CHECK(strstr(c.buf, "ok:") != NULL, "set password with spaces ok");
    CHECK(strcmp(s.cfg.wifi_password, "pass with spaces") == 0,
          "password preserves internal spaces");
}

static void test_save_persists(void) {
    unlink(TEST_PATH);
    sd_provision_session_t s;
    sd_provision_init(&s);
    struct capture c;
    run(&s, "set device_name bench-unit", &c);
    run(&s, "save", &c);
    CHECK(strstr(c.buf, "saved") != NULL, "save reports ok");
    CHECK(s.dirty == 0, "save clears dirty");

    /* A fresh load must see the saved value. */
    sd_config_t reloaded;
    sd_config_load(&reloaded);
    CHECK(strcmp(reloaded.device_name, "bench-unit") == 0, "save persisted");
}

static void test_reset_and_errors(void) {
    sd_provision_session_t s;
    sd_provision_init(&s);
    struct capture c;

    run(&s, "set device_name temp", &c);
    run(&s, "reset", &c);
    CHECK(strcmp(s.cfg.device_name, "sudonit-glasses") == 0, "reset -> defaults");

    run(&s, "bogus", &c);
    CHECK(strstr(c.buf, "unknown command") != NULL, "unknown command error");
    run(&s, "set nope x", &c);
    CHECK(strstr(c.buf, "invalid") != NULL, "invalid key error");
    run(&s, "set server_port 99999", &c);
    CHECK(strstr(c.buf, "invalid") != NULL, "invalid port rejected");
    run(&s, "", &c);
    CHECK(c.buf[0] == '\0', "blank line produces no output");
}

int main(void) {
    setenv("SUDONIT_CONFIG_PATH", TEST_PATH, 1);

    test_help_lists_commands();
    test_set_get_show_mask();
    test_password_with_spaces();
    test_save_persists();
    test_reset_and_errors();

    unlink(TEST_PATH);

    if (g_failures == 0) {
        printf("all provisioning tests passed\n");
        return 0;
    }
    fprintf(stderr, "%d provisioning test(s) failed\n", g_failures);
    return 1;
}
