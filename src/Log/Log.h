#ifndef LOG_LOG_H
#define LOG_LOG_H

#include <stdarg.h>

#include <Arduino.h>

#include "../Config/Config.h"

/**
 * @brief Print an informational log message.
 * @param format printf-like format string.
 * @return ReturnCode_t RET_OK when a message is written.
 */
ReturnCode_t SystemLog(const char *format, ...);

/**
 * @brief Print a warning log message.
 * @param format printf-like format string.
 * @return ReturnCode_t RET_OK when a message is written.
 */
ReturnCode_t SystemWarn(const char *format, ...);

/**
 * @brief Print an error log message.
 * @param format printf-like format string.
 * @return ReturnCode_t RET_OK when a message is written.
 */
ReturnCode_t SystemErr(const char *format, ...);

#endif  // LOG_LOG_H
