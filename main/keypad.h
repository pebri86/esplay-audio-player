/**
 * @file keypad.h
 * @brief Keypad input handling header file.
 *
 * Defines keypad button constants and functions for keypad initialization, sampling, and debouncing.
 */

#pragma once

#include <stdint.h>

/** Keypad button bitmask constants. */
enum {
    KEYPAD_START = 1,   /**< Start button. */
    KEYPAD_SELECT = 2,  /**< Select button. */
    KEYPAD_UP = 4,      /**< Up button. */
    KEYPAD_DOWN = 8,    /**< Down button. */
    KEYPAD_LEFT = 16,   /**< Left button. */
    KEYPAD_RIGHT = 32,  /**< Right button. */
    KEYPAD_A = 64,      /**< A button. */
    KEYPAD_B = 128,     /**< B button. */
    KEYPAD_MENU = 256,  /**< Menu button. */
    KEYPAD_L = 512,     /**< L button. */
    KEYPAD_R = 1024,    /**< R button. */
};

/**
 * @brief Initialize the keypad subsystem.
 *
 * Sets up GPIO pins for keypad input.
 */
void keypad_init(void);

/**
 * @brief Sample the current keypad state.
 *
 * Reads the GPIO pins and returns a bitmask of pressed buttons.
 *
 * @return Bitmask of currently pressed buttons.
 */
uint16_t keypad_sample(void);

/**
 * @brief Debounce keypad input and detect changes.
 *
 * Applies debouncing logic to the sample and updates the changes bitmask.
 *
 * @param sample Current keypad sample.
 * @param changes Pointer to store the bitmask of changed buttons.
 * @return Debounced keypad state.
 */
uint16_t keypad_debounce(uint16_t sample, uint16_t *changes);
