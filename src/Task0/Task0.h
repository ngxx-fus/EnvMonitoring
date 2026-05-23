#ifndef TASK0_TASK0_H
#define TASK0_TASK0_H

#include <Arduino.h>

#include "../Config/Config.h"

/**
 * @brief Get averaged humidity value from history slot.
 * @param index 0..4, where 0 is the latest.
 * @return float Averaged humidity or NAN if unavailable.
 */
float DHT11_GetHumidity(uint8_t index);

/**
 * @brief Get averaged temperature value from history slot.
 * @param index 0..4, where 0 is the latest.
 * @return float Averaged temperature or NAN if unavailable.
 */
float DHT11_GetTemperature(uint8_t index);

/**
 * @brief Initialize DHT11 driver and task state.
 * @return ReturnCode_t Status code.
 */
ReturnCode_t Task0_Init(void);

/**
 * @brief Periodic runtime for DHT11 acquisition.
 * @return ReturnCode_t Status code.
 */
ReturnCode_t Task0_Runtime(void);

#endif  // TASK0_TASK0_H
