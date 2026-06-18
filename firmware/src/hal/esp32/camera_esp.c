/* ESP32 camera backend — OV5640 over DVP via the esp32-camera component.
 *
 * Target board: "ESP32-S3 N16R8 CAM + OV5640" (AliExpress A+A+A class). N16R8 =
 * 16 MB flash + 8 MB octal (OPI) PSRAM, which matches sdkconfig.defaults
 * (CONFIG_ESPTOOLPY_FLASHSIZE_16MB + CONFIG_SPIRAM_MODE_OCT). The JPEG frame
 * buffer lives in PSRAM (CAMERA_FB_IN_PSRAM) — the camera cannot work without it.
 *
 * HONESTY / STATUS:
 *   - This is the REAL driver, not a fake: on hardware it returns actual frames.
 *   - It has NEVER been run on silicon. The pin map below is the COMMON pinout
 *     for this board class (Freenove ESP32-S3-CAM layout, which most generic
 *     N16R8 S3-CAM clones copy) but is UNVERIFIED for this exact listing.
 *     >>> Day-one task #1: confirm every pin against the seller's diagram. <<<
 *     If it differs, it almost certainly matches another esp32-camera preset
 *     (see esp32-camera's camera_pins.h board definitions).
 *   - Compiled only when SUDONIT_CAMERA_DRIVER is defined (and the esp32-camera
 *     managed component is present). Without the flag this stays an honest stub
 *     returning SD_ERR_UNSUPPORTED, so the default build needs no camera
 *     component and boot is unaffected.
 */
#include "sudonit/hal/camera.h"

#ifdef SUDONIT_CAMERA_DRIVER

#include "esp_camera.h"

#include "sudonit/log.h"

static const char *TAG = "camera_esp";

/* --- Pin map (ASSUMPTION — verify against the board before first flash) ------
 * DVP 8-bit parallel bus + SCCB (I2C-like) control + XCLK/PCLK/VSYNC/HREF.
 * PWDN/RESET are tied/unused on most of these boards (-1 = not driven). */
#define CAM_PIN_PWDN  -1
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK  15
#define CAM_PIN_SIOD   4 /* SCCB SDA */
#define CAM_PIN_SIOC   5 /* SCCB SCL */
#define CAM_PIN_D7    16 /* Y9 */
#define CAM_PIN_D6    17 /* Y8 */
#define CAM_PIN_D5    18 /* Y7 */
#define CAM_PIN_D4    12 /* Y6 */
#define CAM_PIN_D3    10 /* Y5 */
#define CAM_PIN_D2     8 /* Y4 */
#define CAM_PIN_D1     9 /* Y3 */
#define CAM_PIN_D0    11 /* Y2 */
#define CAM_PIN_VSYNC  6
#define CAM_PIN_HREF   7
#define CAM_PIN_PCLK  13

/* One outstanding frame at a time: the device captures, sends, then releases
 * before the next capture (see sd_device_run_uplink). We hold the framebuffer
 * handle so sd_camera_release can hand it back to the driver. */
static camera_fb_t *s_fb;
static int s_inited;

sd_err_t sd_camera_init(void) {
    if (s_inited) {
        return SD_OK;
    }

    camera_config_t config = {
        .pin_pwdn = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,
        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,

        .xclk_freq_hz = 20000000, /* 20 MHz — safe default for OV5640 */
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        /* Conservative first-bring-up settings: hardware JPEG at a modest frame
         * size keeps a frame comfortably inside PSRAM and the pixel clock within
         * DMA bandwidth. Resolution can be raised once the path is proven. */
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_SVGA, /* 800x600 */
        .jpeg_quality = 12,           /* 0..63, lower = better quality/bigger */
        .fb_count = 1,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        /* Most common causes: wrong pin map, no PSRAM, or sensor not detected. */
        SD_LOGE(TAG, "esp_camera_init failed (0x%x) — check pin map / PSRAM / wiring",
                (unsigned)err);
        return SD_ERR_IO;
    }

    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor) {
        SD_LOGI(TAG, "camera sensor PID=0x%x detected", (unsigned)sensor->id.PID);
    }
    s_inited = 1;
    return SD_OK;
}

sd_err_t sd_camera_capture(sd_image_t *out) {
    if (!out) {
        return SD_ERR_INVALID;
    }
    if (!s_inited) {
        return SD_ERR_UNSUPPORTED;
    }
    if (s_fb) {
        /* Previous frame not released — protocol/app bug. */
        return SD_ERR_INVALID;
    }

    s_fb = esp_camera_fb_get();
    if (!s_fb) {
        SD_LOGE(TAG, "esp_camera_fb_get returned NULL (capture failed)");
        return SD_ERR_IO;
    }
    if (s_fb->format != PIXFORMAT_JPEG) {
        /* The app contract is JPEG bytes; anything else is a config error. */
        esp_camera_fb_return(s_fb);
        s_fb = NULL;
        return SD_ERR_IO;
    }

    out->media_type = "image/jpeg";
    out->data = s_fb->buf; /* owned by the driver until sd_camera_release */
    out->len = s_fb->len;
    out->width = (uint16_t)s_fb->width;
    out->height = (uint16_t)s_fb->height;
    return SD_OK;
}

void sd_camera_release(sd_image_t *img) {
    if (img) {
        img->data = NULL;
        img->len = 0;
    }
    if (s_fb) {
        esp_camera_fb_return(s_fb);
        s_fb = NULL;
    }
}

const char *sd_camera_backend(void) { return "ov5640"; }

#else /* !SUDONIT_CAMERA_DRIVER — honest stub until the driver is enabled */

sd_err_t sd_camera_init(void) {
    return SD_OK; /* no-op; real init configures the OV5640 over SCCB/DVP */
}

sd_err_t sd_camera_capture(sd_image_t *out) {
    (void)out;
    return SD_ERR_UNSUPPORTED; /* build with -DSUDONIT_CAMERA_DRIVER=1 + esp32-camera */
}

void sd_camera_release(sd_image_t *img) {
    (void)img; /* nothing allocated by the stub */
}

const char *sd_camera_backend(void) { return "ov5640 (stub)"; }

#endif /* SUDONIT_CAMERA_DRIVER */
