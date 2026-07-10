#include <Arduino.h>

#include "Config/Config.h"
#include "WiFi.h"
#include "Log/Log.h"
#include "Common/TaskCommon.h"
#include "DisplayData/DisplayData.h" // This path should already be correct, but double-check
#include "Task0/Task0.h"
#include "Task1/Task1.h"
#include "Task2/Task2.h"
#include "Task3/Task3.h"
#include "Task4/Task4.h"

/**
 * @brief Arduino setup entry.
 */
void setup() {
  Serial.begin(115200);
  delay(100);

  Task_SetSystemState(eAPP_STAT_INIT);
  DisplayData_Init();

  // --- Init sequence with display feedback ---
  const char *helloLines[] = {"", "HELLO", ""};
  Task2_DrawScreenLines(helloLines);
  delay(1000);

  // Initialize hardware tasks first
  if (Task0_Init() != RET_OK) {
    SystemErr("Task0_Init failed");
  }
  if (Task2_Init() != RET_OK) {
    SystemErr("Task2_Init failed");
  }
  if (Task3_Init() != RET_OK) {
    SystemErr("Task3_Init failed");
  }

  // Initialize DS1302
  const char *ds1302Lines[] = {"DS1302", "Init...", ""};
  Task2_DrawScreenLines(ds1302Lines);
  delay(100);
  if (Task4_Init() != RET_OK) {
    const char *ds1302FailLines[] = {"DS1302", "Fail!", ""};
    Task2_DrawScreenLines(ds1302FailLines);
    SystemErr("Task4_Init failed");
    delay(1000);
  }
  if (!DS1302_IsAvailable()) {
    const char *ds1302FailLines[] = {"DS1302", "Fail!", ""};
    Task2_DrawScreenLines(ds1302FailLines);
    delay(1000);
  }

  // Initialize Wi-Fi and NTP
  Task1_Init(); // Start Wi-Fi connection attempt
  const char *wifiLines[] = {"Wi-Fi", "Connect...", ""};
  Task2_DrawScreenLines(wifiLines);

  uint32_t wifiStartTime = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - wifiStartTime < 5000)) {
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    SystemLog("Wi-Fi Connected.");
    char ipLine[32];
    snprintf(ipLine, sizeof(ipLine), "IP: %s", WiFi.localIP().toString().c_str());
    const char *ipLines[] = {"Wi-Fi", ipLine, ""};
    Task2_DrawScreenLines(ipLines);
    delay(2000);
  } else {
    SystemWarn("Wi-Fi connection timed out.");
    const char *timeoutLines[] = {"Wi-Fi", "Timeout", ""};
    Task2_DrawScreenLines(timeoutLines);
    delay(1000);
  }

  SystemLog("Boot sequence complete. Starting main loop.");
  Task_SetSystemState(eAPP_STAT_RUN_MIX); // Start in MIX mode
}

/**
 * @brief Arduino main runtime loop.
 */
void loop() {
  (void)Task0_Runtime();
  (void)Task1_Runtime();
  (void)Task4_Runtime();
  (void)Task3_Runtime();
  (void)Task2_Runtime();
  delay(10);
}