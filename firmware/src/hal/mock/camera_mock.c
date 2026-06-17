/* Mock camera backend (host build).
 *
 * Returns a synthetic gradient buffer so the capture path can be exercised
 * without an OV5640. Real bytes/format don't matter here — this seam only
 * proves "init -> capture -> release" works and is wired into the device loop.
 * On hardware, camera_esp.c replaces this file and returns a real JPEG.
 */
#include "sudonit/hal/camera.h"

#include <stdlib.h>
#include <string.h>

#define MOCK_W 64
#define MOCK_H 48

static int s_initialized;

sd_err_t sd_camera_init(void) {
    s_initialized = 1;
    return SD_OK;
}

sd_err_t sd_camera_capture(sd_image_t *out) {
    if (!out) {
        return SD_ERR_INVALID;
    }
    if (!s_initialized) {
        return SD_ERR_AGAIN;
    }

    const size_t len = (size_t)MOCK_W * MOCK_H * 3; /* RGB */
    uint8_t *buf = malloc(len);
    if (!buf) {
        return SD_ERR_NO_MEM;
    }
    /* A simple deterministic gradient — recognisable, verifiable, cheap. */
    for (int y = 0; y < MOCK_H; ++y) {
        for (int x = 0; x < MOCK_W; ++x) {
            uint8_t *px = &buf[((size_t)y * MOCK_W + x) * 3];
            px[0] = (uint8_t)(x * 255 / MOCK_W);
            px[1] = (uint8_t)(y * 255 / MOCK_H);
            px[2] = 128;
        }
    }

    out->media_type = "image/x-mock-rgb";
    out->data = buf;
    out->len = len;
    out->width = MOCK_W;
    out->height = MOCK_H;
    return SD_OK;
}

void sd_camera_release(sd_image_t *img) {
    if (img && img->data) {
        free(img->data);
        img->data = NULL;
        img->len = 0;
    }
}

const char *sd_camera_backend(void) { return "mock"; }
