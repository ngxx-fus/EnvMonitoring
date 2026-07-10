#include "Task2.h"

#include <WiFi.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <math.h>

#include "../Log/Log.h"
#include "../Task0/Task0.h"
#include "../Task1/Task1.h"
#include "Task4/Task4.h"

namespace {

constexpr uint32_t kClockSyncThresholdYear = 2025U;
constexpr uint32_t kNetProbeIntervalMs = 5000U;

enum class DisplayMode : uint8_t {
    ENV_INFO = 0,
    TIME_INFO,
};

enum class NetState : uint8_t {
  WIFI_OFF = 0,
  INTERNET_DOWN,
  SERVER_WAIT,
  TIME_READY,
};

struct ClockState {
  uint16_t year;
  uint8_t mon;
  uint8_t day;
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
};

U8G2_SH1106_128X64_NONAME_F_HW_I2C g_Display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
DisplayMode g_Mode = DisplayMode::ENV_INFO;
uint32_t g_LastSwitchMs = 0U;
uint32_t g_LastNetProbeMs = 0U;
NetState g_NetState = NetState::WIFI_OFF;
ClockState g_InternalClock = {2000U, 1U, 1U, 0U, 0U, 0U};
uint32_t g_InternalClockBaseMs = 0U;
bool g_NtpHostResolved = false;

/* Display behavior controlled by BOOT0 button */
enum class DisplayBehavior : uint8_t { SWAP = 0, FIX_ENV, FIX_TIME };
DisplayBehavior g_DisplayBehavior = DisplayBehavior::SWAP;

/* BOOT0 button debounce state */
uint8_t g_BootLastState = HIGH;
uint32_t g_BootLastChangeMs = 0U;
constexpr uint32_t kBootDebounceMs = 50U;

bool IsNtpTimeValid(void) {
  const uint16_t actualYear = (g_Year < 100U) ? static_cast<uint16_t>(2000U + g_Year) : g_Year;
  return (actualYear >= kClockSyncThresholdYear);
}

ClockState SnapshotTask1Time(void) {
  ClockState clock = g_InternalClock;
  const uint16_t actualYear = (g_Year < 100U) ? static_cast<uint16_t>(2000U + g_Year) : g_Year;
  if (actualYear >= kClockSyncThresholdYear) {
    clock.year = actualYear;
    clock.mon = g_Mon;
    clock.day = g_Day;
    clock.hour = g_Hour;
    clock.min = g_Min;
    clock.sec = g_Sec;
  }
  return clock;
}

uint8_t DaysInMonth(uint16_t year, uint8_t month) {
  switch (month) {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
      return 31U;
    case 4:
    case 6:
    case 9:
    case 11:
      return 30U;
    case 2:
      return (((year % 4U) == 0U) && (((year % 100U) != 0U) || ((year % 400U) == 0U))) ? 29U
                                                                                            : 28U;
    default:
      return 30U;
  }
}

void IncrementOneSecond(ClockState &clock) {
  ++clock.sec;
  if (clock.sec < 60U) {
    return;
  }
  clock.sec = 0U;
  ++clock.min;
  if (clock.min < 60U) {
    return;
  }
  clock.min = 0U;
  ++clock.hour;
  if (clock.hour < 24U) {
    return;
  }
  clock.hour = 0U;
  ++clock.day;
  if (clock.day <= DaysInMonth(clock.year, clock.mon)) {
    return;
  }
  clock.day = 1U;
  ++clock.mon;
  if (clock.mon <= 12U) {
    return;
  }
  clock.mon = 1U;
  ++clock.year;
}

void AdvanceInternalClock(uint32_t nowMs) {
  if (g_InternalClockBaseMs == 0U) {
    g_InternalClockBaseMs = nowMs;
    return;
  }

  uint32_t elapsedMs = nowMs - g_InternalClockBaseMs;
  while (elapsedMs >= 1000U) {
    IncrementOneSecond(g_InternalClock);
    g_InternalClockBaseMs += 1000U;
    elapsedMs -= 1000U;
  }
}

void SyncInternalClockFromNtp(void) {
  if (!IsNtpTimeValid()) {
    return;
  }

  g_InternalClock.year = (g_Year < 100U) ? static_cast<uint16_t>(2000U + g_Year) : g_Year;
  g_InternalClock.mon = g_Mon;
  g_InternalClock.day = g_Day;
  g_InternalClock.hour = g_Hour;
  g_InternalClock.min = g_Min;
  g_InternalClock.sec = g_Sec;
  g_InternalClockBaseMs = millis();
}

NetState DetectNetState(uint32_t nowMs) {
  if (WiFi.status() != WL_CONNECTED) {
    return NetState::WIFI_OFF;
  }

  if (IsNtpTimeValid()) {
    return NetState::TIME_READY;
  }

  if (g_LastNetProbeMs == 0U) {
    return NetState::SERVER_WAIT;
  }

  if ((nowMs - g_LastNetProbeMs) >= kNetProbeIntervalMs) {
    g_LastNetProbeMs = nowMs;
    IPAddress resolvedIp;
    g_NtpHostResolved = WiFi.hostByName(TIME_NTP_SERVER[0], resolvedIp);
  }

  return g_NtpHostResolved ? NetState::SERVER_WAIT : NetState::INTERNET_DOWN;
}

const char *GetStatusText(NetState state) {
  switch (state) {
    case NetState::WIFI_OFF:
      return "Wi-Fi....";
    case NetState::INTERNET_DOWN:
      return "Internet...";
    case NetState::SERVER_WAIT:
      return "Server...";
    case NetState::TIME_READY:
    default:
      return nullptr;
  }
}

void ApplyDisplayTheme(void) {
  g_Display.setDrawColor(OLED128x64_BACKGROUND_COLOR ? 0 : 1);
}

void DrawEnvInfo(void) {
  char timeText[16];
  const ClockState clock = g_InternalClock; // Use the already-synced internal clock

  snprintf(timeText, sizeof(timeText), "TIME: %02u:%02u:%02u", clock.hour, clock.min, clock.sec);
  g_Display.setFont(u8g2_font_ncenB10_tr);
  g_Display.drawStr(OLED_TEXT_LINE_X[0], OLED_TEXT_LINE_Y[0], timeText);

  const float h = DHT11_GetHumidity(0U);
  const float t = DHT11_GetTemperature(0U);
  char envText[24];
  if (isnan(h) || isnan(t)) {
    snprintf(envText, sizeof(envText), "T: NaN");
    g_Display.drawStr(OLED_TEXT_LINE_X[1], OLED_TEXT_LINE_Y[1], envText);
    snprintf(envText, sizeof(envText), "H: NaN");
    g_Display.drawStr(OLED_TEXT_LINE_X[2], OLED_TEXT_LINE_Y[2], envText);
  } else {
    snprintf(envText, sizeof(envText), "T: %.1foC", t);
    g_Display.drawStr(OLED_TEXT_LINE_X[1], OLED_TEXT_LINE_Y[1], envText);
    snprintf(envText, sizeof(envText), "H: %.1f%%", h);
    g_Display.drawStr(OLED_TEXT_LINE_X[2], OLED_TEXT_LINE_Y[2], envText);
  }
}

void DrawTimeInfo(void) {
    const ClockState clock = g_InternalClock; // Use the already-synced internal clock
    const char *statusText = GetStatusText(g_NetState);

    char topTime[20];
    char midText[24];
    char bottomTemp[24];

    g_Display.setFont(u8g2_font_ncenB10_tr);
    /* Top: always show time */
    snprintf(topTime, sizeof(topTime), "TIME: %02u:%02u:%02u", clock.hour, clock.min, clock.sec);

    /* Middle: show date when available, otherwise status text */
    if (statusText != nullptr && !IsNtpTimeValid()) {
        snprintf(midText, sizeof(midText), "%s", statusText);
    } else {
        snprintf(midText, sizeof(midText), "DATE: %02u/%02u/%04u", clock.day, clock.mon, clock.year);
    }

    /* Bottom: temperature */
    const float t = DHT11_GetTemperature(0U);
    if (isnan(t)) {
        snprintf(bottomTemp, sizeof(bottomTemp), "T: NaN");
    } else {
        snprintf(bottomTemp, sizeof(bottomTemp), "T: %.1foC", t);
    }

    g_Display.drawStr(OLED_TEXT_LINE_X[0], OLED_TEXT_LINE_Y[0], topTime);
    g_Display.drawStr(OLED_TEXT_LINE_X[1], OLED_TEXT_LINE_Y[1], midText);
    g_Display.drawStr(OLED_TEXT_LINE_X[2], OLED_TEXT_LINE_Y[2], bottomTemp);
}

}  // namespace

ReturnCode_t Task2_Init(void) {
  // For U8G2 HW_I2C, the address is often auto-detected.
  // If needed, set the 7-bit address like this: g_Display.setI2CAddress(0x3C);
  // Most common 128x64 OLEDs use address 0x3C.
  // U8g2 library expects the 8-bit address, which is (7-bit address << 1).
  g_Display.setI2CAddress(0x3C * 2);

  g_Display.begin();
  if (g_Display.getU8g2() == nullptr) {
    SystemErr("OLED init failed");
    return RET_FAIL;
  }

  // Clear the buffer and display an empty screen
  g_Display.clearBuffer();
  g_Display.sendBuffer();

  g_Display.setFlipMode(OLED128x64_ORIENTAL % 2); // 0=no, 1=180deg
  g_Display.setFontMode(OLED128x64_BACKGROUND_COLOR ? 0 : 1); // 0=solid, 1=transparent

  g_Mode = DisplayMode::ENV_INFO;
  g_LastSwitchMs = millis();
  g_LastNetProbeMs = 0U;
  g_NetState = NetState::WIFI_OFF;
  g_InternalClock = {2000U, 1U, 1U, 0U, 0U, 0U};
  // Sync from RTC right at startup to have a valid time immediately
  if (DS1302_IsAvailable()) {
    DS1302_GetTime(&g_InternalClock.year, &g_InternalClock.mon, &g_InternalClock.day, &g_InternalClock.hour, &g_InternalClock.min, &g_InternalClock.sec);
  }
  g_InternalClockBaseMs = millis();
  g_NtpHostResolved = false;
  /* Configure BOOT0 button pin for runtime mode switching */
  pinMode(BOOT0_PIN, INPUT_PULLUP);
  g_BootLastState = digitalRead(BOOT0_PIN);
  g_BootLastChangeMs = millis();
  g_DisplayBehavior = DisplayBehavior::SWAP;

  SystemLog("Task2 initialized (OLED 128x64)");
  return RET_OK;
}

ReturnCode_t Task2_Runtime(void) {
  const uint32_t nowMs = millis();
  const uint32_t modeDuration =
      (g_Mode == DisplayMode::ENV_INFO) ? DISP_ENV_INFO_SHOW_MS : DISP_TIME_INFO_SHOW_MS;

  g_NetState = DetectNetState(nowMs);

  /* Handle BOOT0 button presses (active low) with debounce and cycle display behavior */
  const uint8_t bootState = digitalRead(BOOT0_PIN);
  if (bootState != g_BootLastState) {
    g_BootLastChangeMs = nowMs;
    g_BootLastState = bootState;
  } else if ((nowMs - g_BootLastChangeMs) >= kBootDebounceMs) {
    static bool bootHandled = false;
    if (bootState == LOW && !bootHandled) {
      /* Button pressed: cycle behavior */
      if (g_DisplayBehavior == DisplayBehavior::SWAP) {
        g_DisplayBehavior = DisplayBehavior::FIX_ENV;
        g_Mode = DisplayMode::ENV_INFO;
        SystemLog("Display behavior: FIX_ENV");
      } else if (g_DisplayBehavior == DisplayBehavior::FIX_ENV) {
        g_DisplayBehavior = DisplayBehavior::FIX_TIME;
        g_Mode = DisplayMode::TIME_INFO;
        SystemLog("Display behavior: FIX_TIME");
      } else {
        g_DisplayBehavior = DisplayBehavior::SWAP;
        g_LastSwitchMs = nowMs;
        SystemLog("Display behavior: SWAP");
      }
      bootHandled = true;
    } else if (bootState == HIGH) {
      bootHandled = false;
    }
  }

  /* Automatic switching only in SWAP mode */
  if (g_DisplayBehavior == DisplayBehavior::SWAP) {
    if ((nowMs - g_LastSwitchMs) >= modeDuration) {
      g_Mode = (g_Mode == DisplayMode::ENV_INFO) ? DisplayMode::TIME_INFO : DisplayMode::ENV_INFO;
      g_LastSwitchMs = nowMs;
    }
  }

  // Time synchronization priority: NTP > RTC > Internal Fallback
  if (IsNtpTimeValid()) {
    // Highest priority: NTP is valid, sync internal clock from it.
    SyncInternalClockFromNtp();
  } else if (DS1302_IsAvailable()) {
    // Second priority: NTP not ready, but RTC is. Get time from RTC.
    DS1302_GetTime(&g_InternalClock.year, &g_InternalClock.mon, &g_InternalClock.day, &g_InternalClock.hour, &g_InternalClock.min, &g_InternalClock.sec);
  } else {
    // Lowest priority: No external time source, advance the internal clock.
    AdvanceInternalClock(nowMs);
  }

  g_Display.firstPage();
  do {
    ApplyDisplayTheme();

    if (g_Mode == DisplayMode::ENV_INFO) {
      DrawEnvInfo();
    } else {
      DrawTimeInfo();
    }
  } while (g_Display.nextPage());

  return RET_OK;
}
