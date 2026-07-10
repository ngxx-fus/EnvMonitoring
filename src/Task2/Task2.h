#ifndef TASK2_TASK2_H
#define TASK2_TASK2_H

#include <Arduino.h>

#include "../Config/Config.h"

/**
 * @brief Initialize OLED 128x64 display.
 * @return ReturnCode_t Status code.
 */
ReturnCode_t Task2_Init(void);

/**
 * @brief Runtime display refresh with automatic UI switching.
 * @return ReturnCode_t Status code.
 */
ReturnCode_t Task2_Runtime(void);

/**
 * @brief Draws up to 3 lines of text on the OLED screen.
 * @param lines An array of 3 C-strings. Use "" for an empty line.
 * @return ReturnCode_t Status code.
 */
ReturnCode_t Task2_DrawScreenLines(const char *lines[3]);

/**
 * @brief Draws a single line of text at a specified line index (0-2).
 * This will clear the rest of the screen.
 * @param index The line number (0, 1, or 2).
 * @param text The text to draw on the specified line.
 * @return ReturnCode_t Status code.
 */
ReturnCode_t Task2_DrawScreenLine(uint8_t index, const char *text);

/**
 * @brief Draws a monochrome bitmap on the screen.
 * @param x The X position of the top-left corner.
 * @param y The Y position of the top-left corner.
 * @param w The width of the bitmap.
 * @param h The height of the bitmap.
 * @param bitmap A pointer to the bitmap data in U8g2/XBM format.
 * @return ReturnCode_t Status code.
 */
ReturnCode_t Task2_DrawBitmapScreen(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *bitmap);

/**
 * @brief Fills the entire screen with a solid color.
 * @param color `true` for white (inverted), `false` for black (default).
 * @return ReturnCode_t Status code.
 */
ReturnCode_t Task2_FillScreen(bool color);

/**
 * @brief Sets the display to auto-update mode.
 * In this mode, Task2 will not generate its own content but will only render
 * what other tasks write into the DisplayData buffer.
 * @param enabled `true` to enable auto-update, `false` to return to normal operation.
 */
void Task2_SetAutoUpdate(bool enabled);

#endif  // TASK2_TASK2_H
