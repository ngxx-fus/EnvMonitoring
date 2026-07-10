#ifndef TASK3_TASK3_H
#define TASK3_TASK3_H

#include "../Config/Config.h"

/**
 * @brief Initialize the OTA (Over-the-Air) update service.
 * This function sets up the necessary callbacks for the update process.
 * @return ReturnCode_t Status code.
 */
ReturnCode_t Task3_Init(void);

/**
 * @brief Runtime handler for OTA updates.
 * This function must be called repeatedly in the main loop to listen for update requests.
 * @return ReturnCode_t Status code.
 */
ReturnCode_t Task3_Runtime(void);

#endif  // TASK3_TASK3_H