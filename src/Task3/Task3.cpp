#include "Task3.h"

#include <ArduinoOTA.h>
#include <WiFi.h>

#include "Log/Log.h"
#include "Common/TaskCommon.h"
#include "../DisplayData/DisplayData.h"
#include "Task2/Task2.h"

namespace {
  bool g_OtaInitialized = false;
  char otaLines[3][32] = {"UPDATE", "OTA", "Wait..."};
}

ReturnCode_t Task3_Init(void) {
  // Hostname for the OTA service
  ArduinoOTA.setHostname("env-monitoring-esp32");

  // Optional: Set a password for updates
  // ArduinoOTA.setPassword("your_password");

  ArduinoOTA.onStart([]() {
    Task_SetSystemState(eAPP_STAT_OTA);
    Task2_SetAutoUpdate(true); // Take control of the display
    SystemLog("OTA: Update starting...");
    const char *lines[] = {otaLines[0], otaLines[1], otaLines[2]};
    Task2_DrawScreenLines(lines);
  });

  ArduinoOTA.onEnd([]() {
    SystemLog("OTA: Update finished.");
    snprintf(otaLines[2], sizeof(otaLines[2]), "Rebooting...");
    const char *lines[] = {otaLines[0], otaLines[1], otaLines[2]};
    Task2_DrawScreenLines(lines);
    // The system will reboot automatically after this.
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    uint8_t percentage = (progress / (total / 100));
    static uint8_t last_percentage = 0;

    if(last_percentage == percentage) return;
    last_percentage = percentage;

    snprintf(otaLines[2], sizeof(otaLines[2]), "Progress: %u%%", percentage);
    SystemLog("OTA %s", otaLines[2]);
    const char *lines[] = {otaLines[0], otaLines[1], otaLines[2]};
    Task2_DrawScreenLines(lines);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Task_SetSystemState(eAPP_STAT_RUN_MIX); // Revert to a running state on error
    Task2_SetAutoUpdate(false); // Give back control to Task2
    Task2_DrawScreenLine(0, "UPDATE");
    Task2_DrawScreenLine(1, "OTA");
    DisplayData_UpdateString(2, "Failed!");

    SystemErr("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) SystemErr("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) SystemErr("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) SystemErr("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) SystemErr("Receive Failed");
    else if (error == OTA_END_ERROR) SystemErr("End Failed");
  });

  SystemLog("Task3 initialized (OTA)");
  return RET_OK;
}

ReturnCode_t Task3_Runtime(void) {
  // Do not handle OTA if the system is not in a running state (e.g., init)
  if (Task_GetSystemState() == eAPP_STAT_INIT) return RET_OK;

  // Start the OTA service only after Wi-Fi is connected
  if (WiFi.status() == WL_CONNECTED) {
    if (!g_OtaInitialized) {
      ArduinoOTA.begin();
      g_OtaInitialized = true;
      SystemLog("OTA: Service started. Ready for updates at IP: %s", WiFi.localIP().toString().c_str());
    }
    ArduinoOTA.handle();
  } else {
    // If Wi-Fi disconnects, reset the flag
    if (g_OtaInitialized) {
      g_OtaInitialized = false;
      SystemLog("OTA: Service stopped due to Wi-Fi disconnection.");
    }
  }
  return RET_OK;
}