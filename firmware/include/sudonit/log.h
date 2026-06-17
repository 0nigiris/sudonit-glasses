/* Sudonit firmware — leveled logging.
 *
 * A thin logging seam. On the host build it writes to stderr; when the ESP-IDF
 * build is added, sd_log() maps to ESP_LOGx without any call sites changing.
 * This is itself a small abstraction so debugging is ready before first boot.
 */
#ifndef SUDONIT_LOG_H
#define SUDONIT_LOG_H

typedef enum {
    SD_LOG_ERROR = 0,
    SD_LOG_WARN,
    SD_LOG_INFO,
    SD_LOG_DEBUG
} sd_log_level_t;

/* Core logging entry point. printf-style; `tag` groups messages by subsystem. */
void sd_log(sd_log_level_t level, const char *tag, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

/* Set the maximum level that will be emitted (default SD_LOG_INFO). */
void sd_log_set_level(sd_log_level_t level);

#define SD_LOGE(tag, ...) sd_log(SD_LOG_ERROR, (tag), __VA_ARGS__)
#define SD_LOGW(tag, ...) sd_log(SD_LOG_WARN, (tag), __VA_ARGS__)
#define SD_LOGI(tag, ...) sd_log(SD_LOG_INFO, (tag), __VA_ARGS__)
#define SD_LOGD(tag, ...) sd_log(SD_LOG_DEBUG, (tag), __VA_ARGS__)

#endif /* SUDONIT_LOG_H */
