/* ESP32 network backend — Wi-Fi STA bring-up.
 *
 * Brings the data-plane network up so the transport can connect: initialize the
 * TCP/IP stack (esp_netif) and the default event loop, start Wi-Fi in station
 * mode with the configured SSID/password, and block until an IP is assigned
 * (or time out). This is intentionally separate from the transport — see
 * sudonit/hal/net.h.
 *
 * Security: the Wi-Fi password is never logged. Only the SSID and a (set)/(unset)
 * marker are printed; the obtained IP is logged on success.
 */
#include "sudonit/hal/net.h"

#include <string.h>

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "sudonit/log.h"

static const char *TAG = "net_esp";

/* Connection lifecycle, signalled from the Wi-Fi/IP event handler. */
#define NET_CONNECTED_BIT BIT0
#define NET_FAILED_BIT    BIT1
#define NET_MAX_RETRY     5
#define NET_TIMEOUT_MS    15000

static EventGroupHandle_t s_net_events;
static int s_retries;
static int s_started; /* guards repeated init / safe sd_net_stop */

static void net_event_handler(void *arg, esp_event_base_t base, int32_t id,
                              void *data) {
    (void)arg;
    (void)data;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retries < NET_MAX_RETRY) {
            ++s_retries;
            SD_LOGW(TAG, "Wi-Fi disconnected; retry %d/%d", s_retries, NET_MAX_RETRY);
            esp_wifi_connect();
        } else {
            xEventGroupSetBits(s_net_events, NET_FAILED_BIT);
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        SD_LOGI(TAG, "got IP " IPSTR, IP2STR(&event->ip_info.ip));
        s_retries = 0;
        xEventGroupSetBits(s_net_events, NET_CONNECTED_BIT);
    }
}

sd_err_t sd_net_start(const sd_config_t *cfg) {
    if (!cfg) {
        return SD_ERR_INVALID;
    }
    if (cfg->wifi_ssid[0] == '\0') {
        SD_LOGE(TAG, "no Wi-Fi SSID configured — provision before starting network");
        return SD_ERR_INVALID;
    }
    if (s_started) {
        return SD_OK;
    }

    SD_LOGI(TAG, "starting Wi-Fi STA: ssid=%s password=%s", cfg->wifi_ssid,
            cfg->wifi_password[0] ? "(set)" : "(unset)");

    s_net_events = xEventGroupCreate();
    if (!s_net_events) {
        return SD_ERR_NO_MEM;
    }

    /* TCP/IP stack + default event loop + default STA netif. */
    if (esp_netif_init() != ESP_OK ||
        esp_event_loop_create_default() != ESP_OK) {
        return SD_ERR_IO;
    }
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&init_cfg) != ESP_OK) {
        return SD_ERR_IO;
    }

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        net_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        net_event_handler, NULL, NULL);

    wifi_config_t wifi_cfg;
    memset(&wifi_cfg, 0, sizeof(wifi_cfg));
    /* config.h caps ssid<=32 and password<=63 (plus NUL), matching 802.11 and
     * the wifi_config_t field sizes, so these copies cannot overflow. */
    strncpy((char *)wifi_cfg.sta.ssid, cfg->wifi_ssid, sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy((char *)wifi_cfg.sta.password, cfg->wifi_password,
            sizeof(wifi_cfg.sta.password) - 1);

    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK ||
        esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg) != ESP_OK ||
        esp_wifi_start() != ESP_OK) {
        return SD_ERR_IO;
    }
    s_started = 1;

    /* Block until connected, failed, or timed out. */
    EventBits_t bits = xEventGroupWaitBits(
        s_net_events, NET_CONNECTED_BIT | NET_FAILED_BIT, pdFALSE, pdFALSE,
        pdMS_TO_TICKS(NET_TIMEOUT_MS));

    if (bits & NET_CONNECTED_BIT) {
        return SD_OK;
    }
    SD_LOGE(TAG, "Wi-Fi did not connect (%s)",
            (bits & NET_FAILED_BIT) ? "auth/assoc failed" : "timeout");
    return SD_ERR_IO;
}

void sd_net_stop(void) {
    if (!s_started) {
        return;
    }
    esp_wifi_disconnect();
    esp_wifi_stop();
    s_started = 0;
}

const char *sd_net_backend(void) {
    return "esp32";
}
