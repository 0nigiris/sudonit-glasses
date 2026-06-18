/* Serial provisioning console — command processor + REPL.
 *
 * Reuses the config subsystem entirely (sd_config_set_field/get_field, save).
 * No transport assumptions: handle_line takes a string and an output sink.
 */
#include "sudonit/provisioning.h"

#include <stdio.h>
#include <string.h>

static const char *KEYS =
    "device_name | wifi_ssid | wifi_password | server_host | server_port";

void sd_provision_init(sd_provision_session_t *s) {
    if (!s) {
        return;
    }
    sd_config_load(&s->cfg);
    s->dirty = 0;
}

/* Pull the next whitespace-delimited token from *pp, terminating it in place.
 * Returns NULL when none remain; *pp is advanced past the token. */
static char *next_token(char **pp) {
    char *s = *pp;
    while (*s == ' ' || *s == '\t') {
        ++s;
    }
    if (*s == '\0') {
        *pp = s;
        return NULL;
    }
    char *tok = s;
    while (*s && *s != ' ' && *s != '\t') {
        ++s;
    }
    if (*s) {
        *s = '\0';
        ++s;
    }
    *pp = s;
    return tok;
}

static void emit(sd_provision_out_fn out, void *ctx, const char *line) {
    if (out) {
        out(ctx, line);
    }
}

static void emitf(sd_provision_out_fn out, void *ctx, const char *key,
                  const char *value) {
    char buf[160];
    snprintf(buf, sizeof(buf), "%-13s = %s", key, value);
    emit(out, ctx, buf);
}

static void cmd_show(const sd_config_t *cfg, sd_provision_out_fn out, void *ctx) {
    emitf(out, ctx, "device_name", cfg->device_name);
    emitf(out, ctx, "wifi_ssid", cfg->wifi_ssid[0] ? cfg->wifi_ssid : "(unset)");
    emitf(out, ctx, "wifi_password", cfg->wifi_password[0] ? "(set)" : "(unset)");
    emitf(out, ctx, "server_host", cfg->server_host[0] ? cfg->server_host : "(unset)");
    char port[8];
    snprintf(port, sizeof(port), "%u", (unsigned)cfg->server_port);
    emitf(out, ctx, "server_port", port);
}

static void cmd_help(sd_provision_out_fn out, void *ctx) {
    emit(out, ctx, "commands:");
    emit(out, ctx, "  show                 print all settings (password masked)");
    emit(out, ctx, "  get <key>            print one setting");
    emit(out, ctx, "  set <key> <value>    change a setting (in memory)");
    emit(out, ctx, "  save                 persist changes to storage");
    emit(out, ctx, "  reset                restore defaults (then 'save' to persist)");
    emit(out, ctx, "  help                 this help");
    char buf[160];
    snprintf(buf, sizeof(buf), "keys: %s", KEYS);
    emit(out, ctx, buf);
}

sd_err_t sd_provision_handle_line(sd_provision_session_t *s, const char *line,
                                  sd_provision_out_fn out, void *ctx) {
    if (!s || !line) {
        return SD_ERR_INVALID;
    }

    char buf[256];
    size_t n = 0;
    while (line[n] && n < sizeof(buf) - 1) {
        buf[n] = line[n];
        ++n;
    }
    buf[n] = '\0';
    /* Trim trailing whitespace / CR / LF. */
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r' ||
                     buf[n - 1] == ' ' || buf[n - 1] == '\t')) {
        buf[--n] = '\0';
    }

    char *p = buf;
    char *cmd = next_token(&p);
    if (!cmd) {
        return SD_OK; /* blank line */
    }

    if (strcmp(cmd, "help") == 0) {
        cmd_help(out, ctx);
    } else if (strcmp(cmd, "show") == 0) {
        cmd_show(&s->cfg, out, ctx);
    } else if (strcmp(cmd, "get") == 0) {
        char *key = next_token(&p);
        if (!key) {
            emit(out, ctx, "error: usage: get <key>");
            return SD_OK;
        }
        if (strcmp(key, "wifi_password") == 0) {
            emit(out, ctx, s->cfg.wifi_password[0] ? "(set)" : "(unset)");
            return SD_OK;
        }
        char value[128];
        if (sd_config_get_field(&s->cfg, key, value, sizeof(value)) != SD_OK) {
            emit(out, ctx, "error: unknown key");
            return SD_OK;
        }
        emit(out, ctx, value);
    } else if (strcmp(cmd, "set") == 0) {
        char *key = next_token(&p);
        /* Value is the remainder (may contain spaces, e.g. a Wi-Fi password). */
        while (*p == ' ' || *p == '\t') {
            ++p;
        }
        if (!key || *p == '\0') {
            emit(out, ctx, "error: usage: set <key> <value>");
            return SD_OK;
        }
        if (sd_config_set_field(&s->cfg, key, p) != SD_OK) {
            emit(out, ctx, "error: invalid key or value");
            return SD_OK;
        }
        s->dirty = 1;
        if (strcmp(key, "wifi_password") == 0) {
            emit(out, ctx, "ok: wifi_password updated (use 'save' to persist)");
        } else {
            char msg[160];
            snprintf(msg, sizeof(msg), "ok: %s set (use 'save' to persist)", key);
            emit(out, ctx, msg);
        }
    } else if (strcmp(cmd, "save") == 0) {
        if (sd_config_save(&s->cfg) != SD_OK) {
            emit(out, ctx, "error: failed to save");
            return SD_OK;
        }
        s->dirty = 0;
        emit(out, ctx, "ok: saved");
    } else if (strcmp(cmd, "reset") == 0) {
        sd_config_defaults(&s->cfg);
        s->dirty = 1;
        emit(out, ctx, "ok: configuration reset to defaults (use 'save' to persist)");
    } else {
        emit(out, ctx, "error: unknown command (type 'help')");
    }
    return SD_OK;
}

/* --- REPL --- */

static void file_sink(void *ctx, const char *line) {
    FILE *out = ctx;
    fputs(line, out);
    fputc('\n', out);
}

void sd_provision_repl(FILE *in, FILE *out) {
    sd_provision_session_t s;
    sd_provision_init(&s);
    fputs("sudonit provisioning console — type 'help'\n", out);
    fflush(out);

    char line[256];
    while (fgets(line, sizeof(line), in)) {
        sd_provision_handle_line(&s, line, file_sink, out);
        fflush(out);
    }
}
