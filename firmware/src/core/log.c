/* Host implementation of the logging seam: writes to stderr.
 * The ESP-IDF build will provide an alternate implementation mapping to
 * ESP_LOGx; call sites (SD_LOGI/W/E/D) stay identical.
 */
#include "sudonit/log.h"

#include <stdarg.h>
#include <stdio.h>

static sd_log_level_t s_max_level = SD_LOG_INFO;

static const char *level_tag(sd_log_level_t level) {
    switch (level) {
        case SD_LOG_ERROR:
            return "E";
        case SD_LOG_WARN:
            return "W";
        case SD_LOG_INFO:
            return "I";
        case SD_LOG_DEBUG:
            return "D";
    }
    return "?";
}

void sd_log_set_level(sd_log_level_t level) { s_max_level = level; }

void sd_log(sd_log_level_t level, const char *tag, const char *fmt, ...) {
    if (level > s_max_level) {
        return;
    }
    fprintf(stderr, "[%s][%s] ", level_tag(level), tag ? tag : "?");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
}
