/* Host tests for the HAL + device loop, exercised against the mock backends.
 *
 * No framework — a tiny assert harness keeps the firmware test dependency-free
 * (and trivially portable to an on-target test later). Run via ctest.
 */
#include <stdio.h>
#include <string.h>

#include "device.h"
#include "sudonit/error.h"
#include "sudonit/hal/audio.h"
#include "sudonit/hal/battery.h"
#include "sudonit/hal/camera.h"
#include "sudonit/hal/mic.h"

static int g_failures;

#define CHECK(cond, msg)                                       \
    do {                                                       \
        if (!(cond)) {                                         \
            fprintf(stderr, "FAIL: %s (%s:%d)\n", (msg),       \
                    __FILE__, __LINE__);                       \
            ++g_failures;                                      \
        }                                                      \
    } while (0)

size_t sd_audio_mock_frames_played(void); /* test hook from audio_mock.c */

static void test_error_strings(void) {
    CHECK(strcmp(sd_strerror(SD_OK), "ok") == 0, "SD_OK string");
    CHECK(sd_strerror(SD_ERR_NO_MEM) != NULL, "error string non-null");
}

static void test_camera(void) {
    CHECK(sd_camera_init() == SD_OK, "camera init");
    sd_image_t img = {0};
    CHECK(sd_camera_capture(&img) == SD_OK, "camera capture");
    CHECK(img.data != NULL && img.len > 0, "image has bytes");
    CHECK(img.media_type != NULL, "image has media_type");
    CHECK(img.width > 0 && img.height > 0, "image has dimensions");
    sd_camera_release(&img);
    CHECK(img.data == NULL, "release nulls the buffer");
    CHECK(sd_camera_capture(NULL) == SD_ERR_INVALID, "capture rejects NULL");
}

static void test_battery(void) {
    CHECK(sd_battery_init() == SD_OK, "battery init");
    uint8_t pct = 255;
    CHECK(sd_battery_read(&pct) == SD_OK, "battery read");
    CHECK(pct <= 100, "battery percent in range");
    CHECK(sd_battery_read(NULL) == SD_ERR_INVALID, "battery read rejects NULL");
}

static void test_audio(void) {
    CHECK(sd_audio_init(16000, 1) == SD_OK, "audio init");
    CHECK(sd_audio_init(0, 1) == SD_ERR_INVALID, "audio init rejects 0 rate");
    static const int16_t pcm[256] = {0};
    CHECK(sd_audio_init(16000, 1) == SD_OK, "audio re-init");
    CHECK(sd_audio_play_pcm(pcm, 256) == SD_OK, "audio play");
    CHECK(sd_audio_mock_frames_played() == 256, "audio counted frames");
}

static void test_mic(void) {
    CHECK(sd_mic_init(16000) == SD_OK, "mic init");
    int16_t buf[128];
    size_t got = 0;
    CHECK(sd_mic_read(buf, 128, &got) == SD_OK, "mic read");
    CHECK(got == 128, "mic returned requested frames");
    CHECK(sd_mic_read(NULL, 128, &got) == SD_ERR_INVALID, "mic rejects NULL");
}

static void test_device_cycle(void) {
    CHECK(sd_device_init() == SD_OK, "device init");
    sd_device_status_t status = {0};
    CHECK(sd_device_capture_cycle(&status) == SD_OK, "capture cycle");
    CHECK(status.image_bytes > 0, "cycle produced an image");
    CHECK(status.battery_percent <= 100, "cycle read battery");
    CHECK(sd_device_capture_cycle(NULL) == SD_ERR_INVALID, "cycle rejects NULL");
}

int main(void) {
    test_error_strings();
    test_camera();
    test_battery();
    test_audio();
    test_mic();
    test_device_cycle();

    if (g_failures == 0) {
        printf("all HAL tests passed\n");
        return 0;
    }
    fprintf(stderr, "%d HAL test(s) failed\n", g_failures);
    return 1;
}
