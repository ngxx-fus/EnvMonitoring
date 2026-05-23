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

#endif  // TASK2_TASK2_H
