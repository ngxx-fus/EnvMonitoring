#include "Task1.h"

#include <WiFi.h>
#include <time.h>

#include "../Log/Log.h"

volatile uint8_t g_Hour = 0U;
volatile uint8_t g_Min = 0U;
volatile uint8_t g_Sec = 0U;
volatile uint16_t g_Year = 2000U;
volatile uint8_t g_Mon = 1U;
volatile uint8_t g_Day = 1U;

namespace {

constexpr uint32_t kWifiConnectTimeoutMs = 15000U;
constexpr uint32_t kWifiRetryIntervalMs = 10000U;
constexpr int32_t kTimezoneOffsetSeconds = 7 * 3600;  // UTC+7
constexpr int32_t kDaylightOffsetSeconds = 0;

bool g_NtpConfigured = false;
uint32_t g_LastSyncMs = 0U;
uint32_t g_LastWifiRetryMs = 0U;

ReturnCode_t EnsureWifiConnected(void) {
  if (WiFi.status() == WL_CONNECTED) {
    return RET_OK;
  }

  const uint32_t nowMs = millis();
  if ((nowMs - g_LastWifiRetryMs) < kWifiRetryIntervalMs) {
    return RET_TIMEOUT;
  }
  g_LastWifiRetryMs = nowMs;

  SystemWarn("Wi-Fi disconnected, reconnecting...");
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  return RET_TIMEOUT;
}

void ConfigureNtp(void) {
  if (TIME_NTP_SERVER_COUNT >= 3U) {
    configTime(kTimezoneOffsetSeconds, kDaylightOffsetSeconds, TIME_NTP_SERVER[0],
               TIME_NTP_SERVER[1], TIME_NTP_SERVER[2]);
  } else if (TIME_NTP_SERVER_COUNT == 2U) {
    configTime(kTimezoneOffsetSeconds, kDaylightOffsetSeconds, TIME_NTP_SERVER[0],
               TIME_NTP_SERVER[1]);
  } else if (TIME_NTP_SERVER_COUNT == 1U) {
    configTime(kTimezoneOffsetSeconds, kDaylightOffsetSeconds, TIME_NTP_SERVER[0]);
  }
  g_NtpConfigured = true;
}

}  // namespace

ReturnCode_t Task1_Init(void) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const uint32_t startMs = millis();
  while ((WiFi.status() != WL_CONNECTED) && ((millis() - startMs) < kWifiConnectTimeoutMs)) {
    delay(250);
  }

  if (WiFi.status() != WL_CONNECTED) {
    SystemWarn("Task1 init: Wi-Fi connect timeout");
    return RET_TIMEOUT;
  }

  ConfigureNtp();
  SystemLog("Task1 initialized (Wi-Fi connected: %s)", WiFi.localIP().toString().c_str());
  return RET_OK;
}

ReturnCode_t Task1_Runtime(void) {
  if (EnsureWifiConnected() != RET_OK) {
    return RET_TIMEOUT;
  }

  if (!g_NtpConfigured) {
    ConfigureNtp();
  }

  const uint32_t nowMs = millis();
  if ((nowMs - g_LastSyncMs) < TIME_UPDATE_INTERVAL_MS) {
    return RET_OK;
  }
  g_LastSyncMs = nowMs;

  struct tm timeInfo;
  if (!getLocalTime(&timeInfo, 300)) {
    SystemWarn("NTP time fetch failed");
    return RET_FAIL;
  }

  g_Hour = static_cast<uint8_t>(timeInfo.tm_hour);
  g_Min = static_cast<uint8_t>(timeInfo.tm_min);
  g_Sec = static_cast<uint8_t>(timeInfo.tm_sec);
  g_Year = static_cast<uint16_t>(timeInfo.tm_year + 1900);
  g_Mon = static_cast<uint8_t>(timeInfo.tm_mon + 1);
  g_Day = static_cast<uint8_t>(timeInfo.tm_mday);
  return RET_OK;
}
