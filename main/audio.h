/**
 * @file audio.h
 * @brief Audio subsystem header file.
 *
 * This file contains declarations for audio initialization, playback, volume control,
 * and sample rate configuration using I2S.
 */

#include <stdio.h>
#include "driver/i2s_std.h"

/** Default audio volume percentage. */
#define AUDIO_VOLUME_DEFAULT 20

/**
 * @brief Initialize the audio subsystem with a specific sample rate.
 *
 * Sets up the I2S interface for audio output at the given sample rate.
 *
 * @param audio_sample_rate Sample rate in Hz (e.g., 44100).
 */
void audio_init(int audio_sample_rate);

/**
 * @brief Submit audio buffer for playback.
 *
 * Sends a buffer of audio samples to the I2S output.
 *
 * @param buf Pointer to the audio buffer (16-bit samples).
 * @param n_frames Number of frames in the buffer.
 * @return 0 on success, non-zero on failure.
 */
int audio_submit(short *buf, int n_frames);

/**
 * @brief Set the audio volume.
 *
 * Adjusts the playback volume as a percentage.
 *
 * @param volume_percent Volume level (0-100).
 * @return 0 on success, non-zero on failure.
 */
int audio_volume_set(int volume_percent);

/**
 * @brief Get the current audio volume.
 *
 * @return Current volume percentage (0-100).
 */
int audio_volume_get();

/**
 * @brief Pause audio playback.
 *
 * @return 0 on success, non-zero on failure.
 */
int audio_pause();

/**
 * @brief Resume audio playback.
 *
 * @return 0 on success, non-zero on failure.
 */
int audio_resume();

/**
 * @brief Set the audio sample rate and configuration.
 *
 * Configures the I2S clock and slot mode for the given sample rate, bit depth, and channel mode.
 *
 * @param rate Sample rate in Hz (e.g., 44100).
 * @param bits Bit depth per sample (e.g., 16, 24, 32).
 * @param ch I2S slot mode (mono or stereo).
 * @return 0 on success, non-zero on failure.
 */
int audio_set_sample_rate(uint32_t rate, uint32_t bits, i2s_slot_mode_t ch);