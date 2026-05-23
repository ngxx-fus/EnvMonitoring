#include "Config.h"

const uint8_t DHT11_DATA_PIN = 19U;
const uint8_t DHT11_MEAS_AVG_NUM = 3U;          // Allowed range: 1..10
const uint32_t DHT11_UPDATE_INTERVAL_MS = 2000; // Min: 500

const uint8_t OLED128x64_SCL_PIN = 22U;  // ESP32 default SCL
const uint8_t OLED128x64_SDA_PIN = 21U;  // ESP32 default SDA
const uint8_t OLED128x64_ORIENTAL = 0U;  // Rotation: 0..3
const bool OLED128x64_BACKGROUND_COLOR = false;

const uint32_t DISP_ENV_INFO_SHOW_MS = 5000;
const uint32_t DISP_TIME_INFO_SHOW_MS = 5000;

const char *const TIME_NTP_SERVER[] = {
    "pool.ntp.org",
    "time.google.com",
    "time.windows.com",
};

const uint8_t TIME_NTP_SERVER_COUNT =
    static_cast<uint8_t>(sizeof(TIME_NTP_SERVER) / sizeof(TIME_NTP_SERVER[0]));
const uint32_t TIME_UPDATE_INTERVAL_MS = 1000;

const char WIFI_SSID[] = "P-209";
const char WIFI_PASSWORD[] = "thinhngo888";
