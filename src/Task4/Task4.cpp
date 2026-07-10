#include "Task4.h"

#include <RtcDS1302.h>
#include <ThreeWire.h>

#include "Log/Log.h"
#include "Task1/Task1.h"

namespace {

ThreeWire g_Ds1302Wire(DS1302_DAT_PIN, DS1302_CLK_PIN, DS1302_RST_PIN);
RtcDS1302<ThreeWire> g_Rtc(g_Ds1302Wire);

bool g_RtcOk = false;
uint32_t g_LastNtpSyncCheckMs = 0;
constexpr uint32_t kRtcNtpSyncIntervalMs = 60000;  // 1 minute

}  // namespace

ReturnCode_t Task4_Init(void) {
  g_Rtc.Begin();

  if (!g_Rtc.IsDateTimeValid()) {
    SystemWarn("DS1302: Invalid time, waiting for NTP sync.");
    // Set a default time to signify it's running but needs sync
    g_Rtc.SetDateTime(RtcDateTime(2000, 1, 1, 0, 0, 0));
  }

  if (g_Rtc.GetIsWriteProtected()) {
    SystemLog("DS1302: Disabling write protection.");
    g_Rtc.SetIsWriteProtected(false);
  }

  if (!g_Rtc.GetIsRunning()) {
    SystemWarn("DS1302: RTC was not running, starting now.");
    g_Rtc.SetIsRunning(true);
  }

  // At this point, the RTC module itself is responsive.
  // We can consider it "OK" for providing time, even if the time is not yet NTP-synced.
  g_RtcOk = true;

  RtcDateTime now = g_Rtc.GetDateTime();
  if (now.Year() < 2024) {
    SystemWarn("DS1302: Time is not current (%04u), waiting for NTP sync.", now.Year());
  }

  SystemLog("Task4 initialized (DS1302 RTC available: %s)", g_RtcOk ? "Yes" : "No");
  return RET_OK;
}

ReturnCode_t Task4_Runtime(void) {
  const uint32_t nowMs = millis();

  // Periodically check if we need to sync RTC from NTP
  if ((nowMs - g_LastNtpSyncCheckMs) < kRtcNtpSyncIntervalMs) {
    return RET_OK;
  }
  g_LastNtpSyncCheckMs = nowMs;

  const uint16_t ntpYear = (g_Year < 100U) ? static_cast<uint16_t>(2000U + g_Year) : g_Year;
  if (ntpYear < 2025) {
    // NTP time not valid, nothing to do
    return RET_OK;
  }

  RtcDateTime rtcNow = g_Rtc.GetDateTime();

  // Compare NTP time with RTC time, allow a small difference
  // To avoid complex epoch conversion, we just check if the minute is different.
  // This is a simple but effective way to detect drift or mismatch.
  if (rtcNow.Year() != ntpYear || rtcNow.Month() != g_Mon || rtcNow.Day() != g_Day ||
      rtcNow.Hour() != g_Hour || rtcNow.Minute() != g_Min) {
    SystemLog("DS1302: NTP time differs, updating RTC...");
    RtcDateTime ntpTime(ntpYear, g_Mon, g_Day, g_Hour, g_Min, g_Sec);
    g_Rtc.SetDateTime(ntpTime);
    g_RtcOk = true;  // RTC is now considered OK
  }

  return RET_OK;
}

bool DS1302_IsAvailable(void) {
  return g_RtcOk;
}

void DS1302_GetTime(uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *hour, uint8_t *minute,
                    uint8_t *second) {
  if (!g_RtcOk) {
    return;
  }

  RtcDateTime now = g_Rtc.GetDateTime();

  if (year) *year = now.Year();
  if (month) *month = now.Month();
  if (day) *day = now.Day();
  if (hour) *hour = now.Hour();
  if (minute) *minute = now.Minute();
  if (second) *second = now.Second();
}