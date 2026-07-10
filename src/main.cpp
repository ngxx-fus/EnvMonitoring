#include <Arduino.h>

#include "Config/Config.h"
#include "Log/Log.h"
#include "Task0/Task0.h"
#include "Task1/Task1.h"
#include "Task2/Task2.h"
#include "Task4/Task4.h"

/**
 * @brief Arduino setup entry.
 */
void setup() {
  Serial.begin(115200);
  delay(100);

  SystemLog("System booting...");

  if (Task0_Init() != RET_OK) {
    SystemErr("Task0_Init failed");
  }

  // Init RTC module before other tasks that may use time
  if (Task4_Init() != RET_OK) {
    SystemErr("Task4_Init failed");
  }

  if (Task1_Init() != RET_OK) {
    SystemWarn("Task1_Init failed, runtime will retry");
  }

  if (Task2_Init() != RET_OK) {
    SystemErr("Task2_Init failed");
  }
}

/**
 * @brief Arduino main runtime loop.
 */
void loop() {
  (void)Task0_Runtime();
  (void)Task1_Runtime();
  (void)Task4_Runtime();
  (void)Task2_Runtime();
  delay(10);
}