#ifndef TASK4_TASK4_H
#define TASK4_TASK4_H

#include <Arduino.h>

#include "../Config/Config.h"

/**
 * @brief Initialize DS1302 RTC module.
 * @return ReturnCode_t Status code.
 */
ReturnCode_t Task4_Init(void);

/**
 * @brief Runtime task for DS1302, handles periodic sync with NTP.
 * @return ReturnCode_t Status code.
 */
ReturnCode_t Task4_Runtime(void);

/**
 * @brief Check if the DS1302 module is available and running.
 * @return true if RTC is available, false otherwise.
 */
bool DS1302_IsAvailable(void);

/**
 * @brief Get the current time from the DS1302 module.
 * @param[out] year Pointer to store the year.
 * @param[out] month Pointer to store the month.
 * @param[out] day Pointer to store the day.
 * @param[out] hour Pointer to store the hour.
 * @param[out] minute Pointer to store the minute.
 * @param[out] second Pointer to store the second.
 */
void DS1302_GetTime(uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *hour, uint8_t *minute,
                    uint8_t *second);

#endif  // TASK4_TASK4_H