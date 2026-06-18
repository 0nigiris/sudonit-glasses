/* ESP-IDF entry point.
 *
 * Proves the shared firmware code compiles and runs on the ESP32-S3 target: it
 * boots, reports the active HAL backends, and exercises the app -> HAL path.
 * The peripheral drivers in this build are honest stubs (SD_ERR_UNSUPPORTED),
 * so the capture cycle is expected to report an unsupported peripheral until
 * the real drivers land — that's the demonstration, not a failure.
 */
#include "nvs_flash.h"

#include "device.h"
#include "sudonit/config.h"
#include "sudonit/hal/audio.h"
#include "sudonit/hal/battery.h"
#include "sudonit/hal/camera.h"
#include "sudonit/hal/mic.h"
#include "sudonit/hal/net.h"
#include "sudonit/log.h"
#include "sudonit/provisioning.h"

#ifdef SUDONIT_NET_SELFTEST
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sudonit/hal/transport.h"
#include "sudonit/protocol/messages.h"
#endif

static const char *TAG = "app_main";

void app_main(void) {
    sd_log_set_level(SD_LOG_DEBUG);

    SD_LOGI(TAG, "Sudonit Smart Glasses firmware — ESP-IDF target boot");
    SD_LOGI(TAG, "HAL backends: camera=%s battery=%s audio=%s mic=%s",
            sd_camera_backend(), sd_battery_backend(), sd_audio_backend(),
            sd_mic_backend());

    /* NVS backs the configuration store; initialize it before loading config. */
    esp_err_t nv = nvs_flash_init();
    if (nv == ESP_ERR_NVS_NO_FREE_PAGES || nv == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    sd_config_t cfg;
    sd_config_load(&cfg);
    /* Never log the password — report only whether it is set. */
    SD_LOGI(TAG, "config: device=%s server=%s:%u ssid=%s password=%s",
            cfg.device_name,
            cfg.server_host[0] ? cfg.server_host : "(unset)",
            (unsigned)cfg.server_port,
            cfg.wifi_ssid[0] ? cfg.wifi_ssid : "(unset)",
            cfg.wifi_password[0] ? "(set)" : "(unset)");

    sd_err_t err = sd_device_init();
    SD_LOGI(TAG, "device init: %s", sd_strerror(err));

    /* Exercise the app->HAL path. Stubs return SD_ERR_UNSUPPORTED — expected. */
    sd_device_status_t status = {0};
    err = sd_device_capture_cycle(&status);
    SD_LOGI(TAG, "capture cycle: %s", sd_strerror(err));

    SD_LOGI(TAG, "boot complete — peripheral drivers are stubs pending hardware");

#ifdef SUDONIT_NET_SELFTEST
    /* Data-plane self-test: bring Wi-Fi up from the provisioned credentials,
     * open a TCP connection to the phone server, and exchange ping/pong. This
     * exercises net_esp + transport_wifi + the protocol layer end-to-end without
     * needing the (still-stubbed) camera. Once the camera driver lands, swapping
     * the ping for sd_device_run_uplink completes the full capture->AI loop with
     * no change to the protocol layer. */
    SD_LOGI(TAG, "net self-test: bringing up Wi-Fi (net backend=%s)", sd_net_backend());
    if (sd_net_start(&cfg) != SD_OK) {
        SD_LOGE(TAG, "net self-test: Wi-Fi bring-up failed");
    } else if (cfg.server_host[0] == '\0') {
        SD_LOGE(TAG, "net self-test: no server_host configured");
    } else {
        /* Reconnect with backoff: at bring-up the phone app may not be listening
         * yet, and Wi-Fi/TCP can drop. Retry a bounded number of times rather
         * than failing on the first attempt. Each connect attempt is itself
         * bounded by the transport's connect timeout. */
        sd_transport_t *t = NULL;
        sd_err_t terr = SD_ERR_IO;
        for (int attempt = 1; attempt <= 5; ++attempt) {
            terr = sd_transport_connect(&t, cfg.server_host, cfg.server_port);
            if (terr == SD_OK) {
                break;
            }
            SD_LOGW(TAG, "net self-test: connect attempt %d/5 to %s:%u failed: %s",
                    attempt, cfg.server_host, (unsigned)cfg.server_port,
                    sd_strerror(terr));
            vTaskDelay(pdMS_TO_TICKS(1000 * attempt)); /* linear backoff */
        }
        if (terr != SD_OK) {
            SD_LOGE(TAG, "net self-test: could not connect after retries");
        } else {
            sd_err_t perr = sd_msg_send_ping(t);
            sd_msg_type_t mtype = SD_MSG_UNKNOWN;
            if (perr == SD_OK) {
                perr = sd_msg_recv(t, &mtype, NULL, 0);
            }
            if (perr == SD_OK && mtype == SD_MSG_PONG) {
                SD_LOGI(TAG, "net self-test: ping/pong OK — data plane is live");
            } else {
                SD_LOGE(TAG, "net self-test: no pong (%s, type=%d)",
                        sd_strerror(perr), (int)mtype);
            }
            sd_transport_close(t);
        }
    }
#endif

#ifdef SUDONIT_PROVISION_CONSOLE
    /* Recovery/provisioning channel: drop into the serial console over the UART.
     * ESP-IDF maps stdio to the console UART, so the same REPL used on the host
     * runs unchanged here. Guarded by a build flag so production boots skip it. */
    SD_LOGI(TAG, "entering provisioning console (SUDONIT_PROVISION_CONSOLE)");
    sd_provision_repl(stdin, stdout);
#endif
}
