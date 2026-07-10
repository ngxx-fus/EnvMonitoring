#ifndef DISPLAY_DATA_H
#define DISPLAY_DATA_H

#include <Arduino.h>

/**
 * @brief Initializes the display data module, including the mutex.
 */
void DisplayData_Init(void);

/**
 * @brief Updates a specific display string in a thread-safe manner.
 * @param index The index of the string to update (0: Time, 1: Line2, 2: Line3).
 * @param newStr The new string content.
 */
void DisplayData_UpdateString(uint8_t index, const char *newStr);

/**
 * @brief Gets a specific display string in a thread-safe manner.
 * @param index The index of the string to get.
 * @param buffer The buffer to copy the string into.
 * @param bufferSize The size of the destination buffer.
 */
void DisplayData_GetString(uint8_t index, char *buffer, size_t bufferSize);

/**
 * @brief Gets all three display strings at once.
 * @param line1 Buffer for line 1.
 * @param line2 Buffer for line 2.
 * @param line3 Buffer for line 3.
 * @param bufferSize The size of each buffer.
 */
void DisplayData_GetAllStrings(char *line1, char *line2, char *line3, size_t bufferSize);

/**
 * @brief Checks if the display content has changed since the last commit.
 * @return true if content has changed, false otherwise.
 */
bool DisplayData_IsChanged(void);

/**
 * @brief Commits the current display buffer to the "old" buffer.
 * This should be called after the screen has been successfully updated.
 */
void DisplayData_Commit(void);

#endif // DISPLAY_DATA_H