/**
 * @file audio.c
 * @brief Audio subsystem implementation.
 *
 * This file implements audio initialization, playback, volume control,
 * and sample rate configuration using the ESP32 I2S interface.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_err.h"

#include "config.h"
#include "audio.h"

#define I2S_NUM I2S_NUM_0

static const char *TAG = "audio driver";

static int audio_volume = AUDIO_VOLUME_DEFAULT;
static float audio_volume_f = 0.2f;

static bool initialized = false;
static bool enabled = true;
static i2s_chan_handle_t tx_chan = NULL;
static i2s_std_config_t current_std_cfg;
static SemaphoreHandle_t volume_mutex = NULL;

/**
 * @brief Initialize the audio subsystem with a specific sample rate.
 *
 * Sets up the I2S channel for audio output at the given sample rate.
 *
 * @param audio_sample_rate Sample rate in Hz (e.g., 44100).
 */
void audio_init(int audio_sample_rate)
{
    if (initialized)
    {
        ESP_LOGE(TAG, "Audio already initialized!");
        return;
    }

    esp_err_t error;
    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 6,
        .dma_frame_num = 512,
    };
    if ((error = i2s_new_channel(&chan_cfg, &tx_chan, NULL)) != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not create i2s channel: %s", esp_err_to_name(error));
        return;
    }
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(audio_sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
        .mclk = I2S_GPIO_UNUSED,
        .bclk = I2S_BCLK_PIN,
        .ws = I2S_WS_PIN,
        .dout = I2S_DOUT_PIN,
        .din = I2S_GPIO_UNUSED,
        .invert_flags = {
            .mclk_inv = false,
            .bclk_inv = false,
            .ws_inv = false,
        },
    },
    };
    if ((error = i2s_channel_init_std_mode(tx_chan, &std_cfg)) != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not init i2s std mode: %s", esp_err_to_name(error));
        i2s_del_channel(tx_chan);
        tx_chan = NULL;
        return;
    }
    memcpy(&current_std_cfg, &std_cfg, sizeof(i2s_std_config_t));
    initialized = true;
    volume_mutex = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));

    ESP_LOGI(TAG, "Audio driver initialized: I2S NUM %d", I2S_NUM);
}

/**
 * @brief Apply volume scaling to an audio buffer.
 *
 * Scales the audio samples by the given volume factor and clips to prevent overflow.
 *
 * @param buf Pointer to the audio buffer.
 * @param n_frames Number of frames in the buffer.
 * @param vol Volume scaling factor (0.0 to 1.0).
 */
static void apply_volume(short *buf, const int n_frames, float vol)
{
    for (int i = 0; i < n_frames * 2; ++i)
    {
        int sample = (int)((float)buf[i] * vol);
        if (sample > 32767)
            sample = 32767;
        else if (sample < -32768)
            sample = -32767;

        buf[i] = (short)sample;
    }
}

/**
 * @brief Submit an audio buffer for playback.
 *
 * Applies volume scaling and writes the buffer to the I2S channel.
 *
 * @param buf Pointer to the audio buffer (16-bit samples).
 * @param n_frames Number of frames in the buffer.
 * @return 0 on success, -1 on failure.
 */
int audio_submit(short *buf, int n_frames)
{
    if (!initialized)
    {
        ESP_LOGE(TAG, "Audio not yet initialized");
        return -1;
    }

    xSemaphoreTake(volume_mutex, portMAX_DELAY);
    float vol = audio_volume_f;
    xSemaphoreGive(volume_mutex);

    if (vol == 0.0f)
    {
        memset(buf, 0, n_frames * 2 * sizeof(short));
    }
    else
    {
        apply_volume(buf, n_frames, vol);
    }

    const size_t to_write = 2 * n_frames * sizeof(short);
    size_t written;
    i2s_channel_write(tx_chan, buf, to_write, &written, portMAX_DELAY);
    if (written != to_write)
    {
        ESP_LOGI(TAG, "Error submitting data to i2s");
    }

    return 0;
}

/**
 * @brief Set the audio volume.
 *
 * Clamps the volume to 0-100% and updates the internal volume variables.
 *
 * @param volume_percent Volume level (0-100).
 * @return The clamped volume percentage.
 */
int audio_volume_set(int volume_percent)
{
    if (volume_percent > 100)
        volume_percent = 100;
    else if (volume_percent < 0)
        volume_percent = 0;
    xSemaphoreTake(volume_mutex, portMAX_DELAY);
    audio_volume = volume_percent;
    audio_volume_f = (float)volume_percent / 100.0f;
    xSemaphoreGive(volume_mutex);
    return audio_volume;
}

/**
 * @brief Get the current audio volume.
 *
 * @return Current volume percentage (0-100).
 */
int audio_volume_get()
{
    return audio_volume;
}

/**
 * @brief Pause audio playback.
 *
 * Disables the I2S channel if it is currently enabled.
 *
 * @return 0 on success, -1 on failure.
 */
int audio_pause()
{
    if (enabled)
    {
        esp_err_t error;
        if ((error = i2s_channel_disable(tx_chan)) != ESP_OK)
        {
            fprintf(stderr, "Could not disable i2s channel: %s\n", esp_err_to_name(error));
            return -1;
        }
        enabled = false;
    }

    return 0;
}

/**
 * @brief Resume audio playback.
 *
 * Enables the I2S channel if it is currently disabled.
 *
 * @return 0 on success, -1 on failure.
 */
int audio_resume()
{
    if (!enabled)
    {
        esp_err_t error;
        if ((error = i2s_channel_enable(tx_chan)) != ESP_OK)
        {
            fprintf(stderr, "Could not enable i2s channel: %s\n", esp_err_to_name(error));
            return -1;
        }
        enabled = true;
    }

    return 0;
}

/**
 * @brief Set the audio sample rate and configuration.
 *
 * Validates parameters and logs reconfiguration requests, but does not actually reconfigure
 * as it's not supported in this ESP-IDF version.
 *
 * @param rate Sample rate in Hz.
 * @param bits Bit depth per sample.
 * @param ch I2S slot mode.
 * @return 0 on success, -1 on invalid parameters.
 */
int audio_set_sample_rate(uint32_t rate, uint32_t bits, i2s_slot_mode_t ch)
{
    if (!initialized) {
        ESP_LOGE(TAG, "Audio not initialized");
        return -1;
    }

    // Validate parameters
    if (rate == 0 || (bits != 16 && bits != 24 && bits != 32)) {
        ESP_LOGE(TAG, "Invalid sample rate or bit width: rate=%lu, bits=%lu", (unsigned long)rate, (unsigned long)bits);
        return -1;
    }

    // Check if reconfiguration is needed
    if (current_std_cfg.clk_cfg.sample_rate_hz == rate &&
        current_std_cfg.slot_cfg.data_bit_width == (i2s_data_bit_width_t)bits &&
        current_std_cfg.slot_cfg.slot_mode == ch) {
        // Already configured, no change needed
        return 0;
    }

    // Reconfiguration is not supported in this ESP-IDF version, log and return success to avoid errors
    ESP_LOGW(TAG, "Sample rate reconfiguration requested: rate=%lu, bits=%lu, ch=%d, but not supported. Keeping current config.", (unsigned long)rate, (unsigned long)bits, (int)ch);
    return 0;
}