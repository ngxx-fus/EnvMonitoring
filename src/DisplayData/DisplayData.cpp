#include "DisplayData.h"

#include <string.h>

namespace {

constexpr uint8_t kDisplayLineCount = 3;
constexpr size_t kDisplayStringSize = 32;

char g_DisplayStrings[kDisplayLineCount][kDisplayStringSize];
char g_DisplayStrings_Old[kDisplayLineCount][kDisplayStringSize];
SemaphoreHandle_t g_DisplayDataMutex = nullptr;

} // namespace

void DisplayData_Init(void) {
    g_DisplayDataMutex = xSemaphoreCreateMutex();
    // Initialize both current and old buffers to empty strings
    for (int i = 0; i < kDisplayLineCount; ++i) {
        g_DisplayStrings[i][0] = '\0';
        g_DisplayStrings_Old[i][0] = '\0';
    }
}

void DisplayData_UpdateString(uint8_t index, const char *newStr) {
    if (index >= kDisplayLineCount || newStr == nullptr) return;

    if (xSemaphoreTake(g_DisplayDataMutex, portMAX_DELAY) == pdTRUE) {
        strncpy(g_DisplayStrings[index], newStr, kDisplayStringSize - 1);
        g_DisplayStrings[index][kDisplayStringSize - 1] = '\0'; // Ensure null termination
        xSemaphoreGive(g_DisplayDataMutex);
    }
}

bool DisplayData_IsChanged(void) {
    bool changed = false;
    if (xSemaphoreTake(g_DisplayDataMutex, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < kDisplayLineCount; ++i) {
            if (strcmp(g_DisplayStrings[i], g_DisplayStrings_Old[i]) != 0) {
                changed = true;
                break;
            }
        }
        xSemaphoreGive(g_DisplayDataMutex);
    }
    return changed;
}

void DisplayData_Commit(void) {
    if (xSemaphoreTake(g_DisplayDataMutex, portMAX_DELAY) == pdTRUE) {
        memcpy(g_DisplayStrings_Old, g_DisplayStrings, sizeof(g_DisplayStrings));
        xSemaphoreGive(g_DisplayDataMutex);
    }
}

void DisplayData_GetString(uint8_t index, char *buffer, size_t bufferSize) {
    if (index >= kDisplayLineCount || buffer == nullptr) return;

    if (xSemaphoreTake(g_DisplayDataMutex, portMAX_DELAY) == pdTRUE) {
        strncpy(buffer, g_DisplayStrings[index], bufferSize - 1);
        buffer[bufferSize - 1] = '\0'; // Ensure null termination
        xSemaphoreGive(g_DisplayDataMutex);
    }
}

void DisplayData_GetAllStrings(char *line1, char *line2, char *line3, size_t bufferSize) {
    if (!line1 || !line2 || !line3) return;

    if (xSemaphoreTake(g_DisplayDataMutex, portMAX_DELAY) == pdTRUE) {
        strncpy(line1, g_DisplayStrings[0], bufferSize - 1);
        line1[bufferSize - 1] = '\0';
        strncpy(line2, g_DisplayStrings[1], bufferSize - 1);
        line2[bufferSize - 1] = '\0';
        strncpy(line3, g_DisplayStrings[2], bufferSize - 1);
        line3[bufferSize - 1] = '\0';
        xSemaphoreGive(g_DisplayDataMutex);
    }
}