#ifndef TASK1_TASK1_H
#define TASK1_TASK1_H

#include <Arduino.h>

#include "../Config/Config.h"

/** @brief Latest synchronized hour (0..23). */
extern volatile uint8_t g_Hour;
/** @brief Latest synchronized minute (0..59). */
extern volatile uint8_t g_Min;
/** @brief Latest synchronized second (0..59). */
extern volatile uint8_t g_Sec;
/** @brief Latest synchronized year (e.g. 2026). */
extern volatile uint16_t g_Year;
/** @brief Latest synchronized month (1..12). */
extern volatile uint8_t g_Mon;
/** @brief Latest synchronized day (1..31). */
extern volatile uint8_t g_Day;

/**
 * @brief Initialize Wi-Fi and NTP client.
 * @return ReturnCode_t Status code.
 */
ReturnCode_t Task1_Init(void);

/**
 * @brief Periodic runtime to refresh date/time values.
 * @return ReturnCode_t Status code.
 */
ReturnCode_t Task1_Runtime(void);

#endif  // TASK1_TASK1_H
