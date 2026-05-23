#include "Task2.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <Wire.h>
#include <math.h>

#include "../Log/Log.h"
#include "../Task0/Task0.h"
#include "../Task1/Task1.h"

namespace {

constexpr uint8_t kScreenWidth = 128U;
constexpr uint8_t kScreenHeight = 64U;
constexpr int8_t kOledResetPin = -1;
constexpr uint8_t kOledI2cAddress = 0x3C;
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

Adafruit_SSD1306 g_Display(kScreenWidth, kScreenHeight, &Wire, kOledResetPin);
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
  if (OLED128x64_BACKGROUND_COLOR) {
    g_Display.fillScreen(SSD1306_WHITE);
    g_Display.setTextColor(SSD1306_BLACK);
  } else {
    g_Display.fillScreen(SSD1306_BLACK);
    g_Display.setTextColor(SSD1306_WHITE);
  }
}

void DrawEnvInfo(void) {
  char timeText[16];
  ClockState clock = g_InternalClock;
  if (IsNtpTimeValid()) {
    clock = SnapshotTask1Time();
    SyncInternalClockFromNtp();
  }

  snprintf(timeText, sizeof(timeText), "%02u:%02u:%02u", clock.hour, clock.min, clock.sec);
  g_Display.setTextSize(2);
  g_Display.setCursor(0, 2);
  g_Display.print(timeText);

  const float h = DHT11_GetHumidity(0U);
  const float t = DHT11_GetTemperature(0U);
  char envText[24];
  g_Display.setTextSize(2);
  if (isnan(h) || isnan(t)) {
    snprintf(envText, sizeof(envText), "T: NaN");
    g_Display.setCursor(0, 24);
    g_Display.print(envText);
    snprintf(envText, sizeof(envText), "H: NaN");
    g_Display.setCursor(0, 48);
    g_Display.print(envText);
  } else {
    snprintf(envText, sizeof(envText), "T: %.1foC", t);
    g_Display.setCursor(0, 24);
    g_Display.print(envText);
    snprintf(envText, sizeof(envText), "H: %.1f%%", h);
    g_Display.setCursor(0, 48);
    g_Display.print(envText);
  }
}

void DrawTimeInfo(void) {
    g_Display.setTextSize(2);

    ClockState clock = g_InternalClock;
    const char *statusText = GetStatusText(g_NetState);
    if (IsNtpTimeValid()) {
        clock = SnapshotTask1Time();
        SyncInternalClockFromNtp();
        statusText = nullptr;
    }

    char topTime[20];
    char midText[24];
    char bottomTemp[24];

    /* Top: always show time */
    snprintf(topTime, sizeof(topTime), "%02u:%02u:%02u", clock.hour, clock.min, clock.sec);

    /* Middle: show date when available, otherwise status text */
    if (statusText != nullptr) {
        snprintf(midText, sizeof(midText), "%s", statusText);
    } else {
        snprintf(midText, sizeof(midText), "%02u/%02u/%04u", clock.day, clock.mon, clock.year);
    }

    /* Bottom: temperature */
    const float t = DHT11_GetTemperature(0U);
    if (isnan(t)) {
        snprintf(bottomTemp, sizeof(bottomTemp), "T: NaN");
    } else {
        snprintf(bottomTemp, sizeof(bottomTemp), "T: %.1foC", t);
    }

    g_Display.setCursor(0, 2);
    g_Display.print(topTime);
    g_Display.setCursor(0, 24);
    g_Display.print(midText);
    g_Display.setCursor(0, 48);
    g_Display.print(bottomTemp);
}

}  // namespace

ReturnCode_t Task2_Init(void) {
  Wire.begin(OLED128x64_SDA_PIN, OLED128x64_SCL_PIN);

  if (!g_Display.begin(SSD1306_SWITCHCAPVCC, kOledI2cAddress)) {
    SystemErr("OLED init failed");
    return RET_FAIL;
  }

  g_Display.setRotation(OLED128x64_ORIENTAL % 4U);
  g_Display.cp437(true);
  g_Display.clearDisplay();
  g_Display.display();

  g_Mode = DisplayMode::ENV_INFO;
  g_LastSwitchMs = millis();
  g_LastNetProbeMs = 0U;
  g_NetState = NetState::WIFI_OFF;
  g_InternalClock = {2000U, 1U, 1U, 0U, 0U, 0U};
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

  if (!IsNtpTimeValid()) {
    AdvanceInternalClock(nowMs);
  } else {
    SyncInternalClockFromNtp();
  }

  ApplyDisplayTheme();

  if (g_Mode == DisplayMode::ENV_INFO) {
    DrawEnvInfo();
  } else {
    DrawTimeInfo();
  }

  g_Display.display();
  return RET_OK;
}
