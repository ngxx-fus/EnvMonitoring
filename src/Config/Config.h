#ifndef CONFIG_CONFIG_H
#define CONFIG_CONFIG_H

#include <Arduino.h>

/**
 * @brief Generic return code for project modules.
 */
typedef enum {
  RET_OK = 0,
  RET_INVALID_ARG,
  RET_FAIL,
  RET_TIMEOUT
} ReturnCode_t;

/** @brief DHT11 GPIO pin. */
extern const uint8_t DHT11_DATA_PIN;
/** @brief Number of raw measurements used for one average value. */
extern const uint8_t DHT11_MEAS_AVG_NUM;
/** @brief DHT11 update interval in milliseconds. */
extern const uint32_t DHT11_UPDATE_INTERVAL_MS;

/** @brief OLED SCL pin (default ESP32 I2C pin). */
extern const uint8_t OLED128x64_SCL_PIN;
/** @brief OLED SDA pin (default ESP32 I2C pin). */
extern const uint8_t OLED128x64_SDA_PIN;
/** @brief OLED rotation: 0..3. */
extern const uint8_t OLED128x64_ORIENTAL;
/** @brief OLED background color: false=black, true=white. */
extern const bool OLED128x64_BACKGROUND_COLOR;

/** @brief Environment information screen duration. */
extern const uint32_t DISP_ENV_INFO_SHOW_MS;
/** @brief Date/time information screen duration. */
extern const uint32_t DISP_TIME_INFO_SHOW_MS;

/** @brief List of NTP servers used for time synchronization. */
extern const char *const TIME_NTP_SERVER[];
/** @brief Number of configured NTP servers. */
extern const uint8_t TIME_NTP_SERVER_COUNT;
/** @brief Time refresh interval in milliseconds. */
extern const uint32_t TIME_UPDATE_INTERVAL_MS;

/** @brief Wi-Fi SSID. */
extern const char WIFI_SSID[];
/** @brief Wi-Fi password. */
extern const char WIFI_PASSWORD[];

#endif  // CONFIG_CONFIG_H
