/* ESP32 audio-output backend — MAX98357A over I2S (standard/Philips, 16-bit).
 *
 * Target: a MAX98357A I2S class-D amp on the "ESP32-S3 N16R8 CAM + OV5640" board.
 * The phone renders speech to PCM and streams it down (see the audio downlink);
 * the glasses just play it, so this driver is a thin I2S-TX path implementing the
 * audio.h contract (16-bit PCM in, sound out).
 *
 * HONESTY / STATUS:
 *   - Real driver (ESP-IDF v5.2 i2s_std API), NOT a fake: on hardware it emits
 *     sound. It has NEVER been run on silicon.
 *   - The I2S pins below are an ASSUMPTION chosen to avoid the camera's DVP pins
 *     and the flash/PSRAM/USB/UART/strapping pins (see docs/AUDIO_BRINGUP.md).
 *     >>> Day-one task: confirm these 3 GPIOs are broken out and free on the
 *         board, and do not clash with the verified camera pin map. <<<
 *   - Compiled only when SUDONIT_AUDIO_DRIVER is defined; without the flag this
 *     stays an honest stub (SD_ERR_UNSUPPORTED) so the default build needs no I2S
 *     driver and boot is unaffected.
 */
#include "sudonit/hal/audio.h"

#ifdef SUDONIT_AUDIO_DRIVER

#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"

#include "sudonit/log.h"

static const char *TAG = "audio_esp";

/* --- I2S pin map (ASSUMPTION — verify against the board) ---------------------
 * MAX98357A needs three signals: BCLK (bit clock), WS/LRC (word select), DIN
 * (data). No MCLK, no data-in. Chosen from the GPIO cluster opposite the camera
 * DVP block; GAIN/SD_MODE on the amp are strapped in hardware (not driven here). */
#define I2S_PIN_BCLK 42
#define I2S_PIN_WS   41
#define I2S_PIN_DOUT 40

static i2s_chan_handle_t s_tx;
static uint32_t s_rate;
static uint8_t s_channels;

static void teardown(void) {
    if (s_tx) {
        i2s_channel_disable(s_tx);
        i2s_del_channel(s_tx);
        s_tx = NULL;
    }
}

sd_err_t sd_audio_init(uint32_t sample_rate, uint8_t channels) {
    if (sample_rate == 0 || channels == 0 || channels > 2) {
        return SD_ERR_INVALID;
    }
    /* The phone may stream different sample rates between turns; only rebuild the
     * channel when the format actually changes. */
    if (s_tx && sample_rate == s_rate && channels == s_channels) {
        return SD_OK;
    }
    teardown();

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    if (i2s_new_channel(&chan_cfg, &s_tx, NULL) != ESP_OK) {
        return SD_ERR_IO;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT,
            channels == 1 ? I2S_SLOT_MODE_MONO : I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_PIN_BCLK,
            .ws = I2S_PIN_WS,
            .dout = I2S_PIN_DOUT,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {.mclk_inv = false, .bclk_inv = false, .ws_inv = false},
        },
    };
    if (i2s_channel_init_std_mode(s_tx, &std_cfg) != ESP_OK ||
        i2s_channel_enable(s_tx) != ESP_OK) {
        SD_LOGE(TAG, "I2S init failed — check pins / clock");
        teardown();
        return SD_ERR_IO;
    }

    s_rate = sample_rate;
    s_channels = channels;
    SD_LOGI(TAG, "I2S TX up: %u Hz, %u ch", (unsigned)sample_rate, (unsigned)channels);
    return SD_OK;
}

sd_err_t sd_audio_play_pcm(const int16_t *samples, size_t frame_count) {
    if (!samples && frame_count > 0) {
        return SD_ERR_INVALID;
    }
    if (!s_tx) {
        return SD_ERR_UNSUPPORTED; /* init not called / failed */
    }
    if (frame_count == 0) {
        return SD_OK;
    }

    size_t bytes = frame_count * s_channels * sizeof(int16_t);
    size_t written = 0;
    esp_err_t err = i2s_channel_write(s_tx, samples, bytes, &written, portMAX_DELAY);
    if (err != ESP_OK || written != bytes) {
        SD_LOGE(TAG, "I2S write failed (%d, %zu/%zu bytes)", (int)err, written, bytes);
        return SD_ERR_IO;
    }
    return SD_OK;
}

const char *sd_audio_backend(void) { return "max98357a"; }

#else /* !SUDONIT_AUDIO_DRIVER — honest stub until the driver is enabled */

sd_err_t sd_audio_init(uint32_t sample_rate, uint8_t channels) {
    if (sample_rate == 0 || channels == 0) {
        return SD_ERR_INVALID;
    }
    return SD_OK; /* no-op; real init configures the I2S TX channel */
}

sd_err_t sd_audio_play_pcm(const int16_t *samples, size_t frame_count) {
    (void)samples;
    (void)frame_count;
    return SD_ERR_UNSUPPORTED; /* build with -DSUDONIT_AUDIO_DRIVER=1 */
}

const char *sd_audio_backend(void) { return "max98357a (stub)"; }

#endif /* SUDONIT_AUDIO_DRIVER */
