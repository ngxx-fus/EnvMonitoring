#include "Task2.h"

#include <WiFi.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <math.h>

#include "../Log/Log.h"
#include "../Task0/Task0.h"
#include "../Task1/Task1.h"
#include "../Common/TaskCommon.h"
#include "../DisplayData/DisplayData.h"
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
ClockState g_InternalClock = {2026U, 1U, 1U, 0U, 0U, 0U};
uint32_t g_InternalClockBaseMs = 0U;
bool g_NtpHostResolved = false;

/* BOOT0 button debounce state */
uint8_t g_BootLastState = HIGH;
uint32_t g_BootLastChangeMs = 0U;
uint32_t g_BootPressStartMs = 0U;
bool g_AutoUpdateEnabled = false;
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

void DrawLines(const char *lines[3]) {
  g_Display.setFont(u8g2_font_ncenB10_tr);
  for (int i = 0; i < OLED_TEXT_LINE_COUNT; ++i) {
    if (lines[i] != nullptr) {
      g_Display.drawStr(OLED_TEXT_LINE_X[i], OLED_TEXT_LINE_Y[i], lines[i]);
    }
  }
}

void UpdateDisplayData(void) {
  char line1[32], line2[32], line3[32];
  const float t = DHT11_GetTemperature(0U);
  const float h = DHT11_GetHumidity(0U);
  AppState_t currentState = Task_GetSystemState();

  // In MIX mode, alternate between TDT and TTH content
  if (currentState == eAPP_STAT_RUN_MIX) {
    if ((millis() / DISP_ENV_INFO_SHOW_MS) % 2 == 0) {
      currentState = eAPP_STAT_RUN_TDT;
    } else {
      currentState = eAPP_STAT_RUN_TTH;
    }
  }

  snprintf(line1, sizeof(line1), "#: %02u:%02u:%02u", g_InternalClock.hour, g_InternalClock.min, g_InternalClock.sec);

  if (currentState == eAPP_STAT_RUN_TDT) {
    snprintf(line2, sizeof(line2), "D: %02u/%02u/%04u", g_InternalClock.day, g_InternalClock.mon, g_InternalClock.year);
    snprintf(line3, sizeof(line3), "T: %.1foC", isnan(t) ? NAN : t);
  } else if (currentState == eAPP_STAT_RUN_TTH) {
    snprintf(line2, sizeof(line2), "T: %.1foC", isnan(t) ? NAN : t);
    snprintf(line3, sizeof(line3), "H: %.1f%%", isnan(h) ? NAN : h);
  }

  DisplayData_UpdateString(0, line1);
  DisplayData_UpdateString(1, line2);
  DisplayData_UpdateString(2, line3);
}

void DrawScreenFromBuffer(void) {
    char line1[32], line2[32], line3[32];
    DisplayData_GetAllStrings(line1, line2, line3, sizeof(line1));
    const char *lines[] = {line1, line2, line3};
    Task2_DrawScreenLines(lines);
    DisplayData_Commit();
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
  g_InternalClock = {2026U, 1U, 1U, 0U, 0U, 0U};
  // Sync from RTC right at startup to have a valid time immediately
  if (DS1302_IsAvailable()) {
    DS1302_GetTime(&g_InternalClock.year, &g_InternalClock.mon, &g_InternalClock.day, &g_InternalClock.hour, &g_InternalClock.min, &g_InternalClock.sec);
  }
  g_InternalClockBaseMs = millis();
  g_NtpHostResolved = false;
  /* Configure BOOT0 button pin for runtime mode switching */
  pinMode(CHANGE_MODE_PIN, INPUT_PULLUP);
  g_BootLastState = digitalRead(CHANGE_MODE_PIN);
  g_BootLastChangeMs = millis();

  SystemLog("Task2 initialized (OLED 128x64)");
  return RET_OK;
}

ReturnCode_t Task2_Runtime(void) {
  // This task is responsible for updating display data in RUN states
  // and handling button presses to change states.
  const AppState_t currentState = Task_GetSystemState();
  if (currentState != eAPP_STAT_RUN_TDT && currentState != eAPP_STAT_RUN_TTH && currentState != eAPP_STAT_RUN_MIX) {
    return RET_OK; // Do nothing if not in a display-active state
  }

  // In AutoUpdate mode, Task2 only handles drawing, not data preparation.
  if (!g_AutoUpdateEnabled) {
    const uint32_t nowMs = millis();
    g_NetState = DetectNetState(nowMs);

    // --- Handle button presses (active low) ---
    const uint8_t bootState = digitalRead(CHANGE_MODE_PIN);
    if (bootState != g_BootLastState) { // State changed
      g_BootLastChangeMs = nowMs;
      g_BootLastState = bootState;
      if (bootState == LOW) { // Button was just pressed
        g_BootPressStartMs = nowMs;
      }
    }

    // --- Process button events after debounce ---
    if ((nowMs - g_BootLastChangeMs) >= kBootDebounceMs) {
      // Long press check (held for 1s)
      if (g_BootLastState == LOW && (nowMs - g_BootPressStartMs >= 1000)) {
        SystemLog("Long press detected, entering OTA mode.");
        Task_SetSystemState(eAPP_STAT_OTA);
        g_BootPressStartMs = 0; // Prevent re-triggering
        g_BootLastState = HIGH; // Fake a release to prevent short press
      }
      // Short press check (released before 1s)
      else if (g_BootLastState == HIGH && g_BootPressStartMs != 0) {
        SystemLog("Short press detected, changing run mode.");
        if (currentState == eAPP_STAT_RUN_TDT) {
          Task_SetSystemState(eAPP_STAT_RUN_TTH);
        } else if (currentState == eAPP_STAT_RUN_TTH) {
          Task_SetSystemState(eAPP_STAT_RUN_MIX);
        } else if (currentState == eAPP_STAT_RUN_MIX) {
          Task_SetSystemState(eAPP_STAT_RUN_TDT);
        }
        g_BootPressStartMs = 0; // Reset press timer
      }
    }

    // --- Time synchronization ---
    if (IsNtpTimeValid()) {
      SyncInternalClockFromNtp();
    } else if (DS1302_IsAvailable()) {
      DS1302_GetTime(&g_InternalClock.year, &g_InternalClock.mon, &g_InternalClock.day, &g_InternalClock.hour, &g_InternalClock.min, &g_InternalClock.sec);
    } else {
      AdvanceInternalClock(nowMs);
    }

    // --- Update shared data buffer ---
    UpdateDisplayData();
  }
  
  // --- Drawing Stage ---
  // Only redraw the screen if the content has actually changed.
  if (DisplayData_IsChanged()) {
    DrawScreenFromBuffer();
  }

  return RET_OK;
}

void Task2_SetAutoUpdate(bool enabled) {
  g_AutoUpdateEnabled = enabled;
}

ReturnCode_t Task2_DrawScreenLine(uint8_t index, const char *text) {
  if (index >= OLED_TEXT_LINE_COUNT) {
    return RET_INVALID_ARG;
  }

  g_Display.firstPage();
  do {
    ApplyDisplayTheme();
    g_Display.setFont(u8g2_font_ncenB10_tr);
    g_Display.drawStr(OLED_TEXT_LINE_X[index], OLED_TEXT_LINE_Y[index], text);
  } while (g_Display.nextPage());

  return RET_OK;
}

ReturnCode_t Task2_DrawBitmapScreen(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *bitmap) {
  if (bitmap == nullptr) {
    return RET_INVALID_ARG;
  }

  g_Display.firstPage();
  do {
    ApplyDisplayTheme();
    g_Display.drawXBM(x, y, w, h, bitmap);
  } while (g_Display.nextPage());

  return RET_OK;
}

ReturnCode_t Task2_FillScreen(bool color) {
  g_Display.firstPage();
  do {
    g_Display.setDrawColor(color ? 1 : 0);
    g_Display.drawBox(0, 0, g_Display.getDisplayWidth(), g_Display.getDisplayHeight());
  } while (g_Display.nextPage());

  return RET_OK;
}

ReturnCode_t Task2_DrawScreenLines(const char *lines[3]) {
  g_Display.firstPage();
  do {
    ApplyDisplayTheme();
    DrawLines(lines);
  } while (g_Display.nextPage());

  return RET_OK;
}
