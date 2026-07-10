#ifndef __APP_COMMON_H__
#define __APP_COMMON_H__

typedef enum e_AppState {
    eAPP_STAT_INIT          = 0,
    eAPP_STAT_RUN_TDT       = 1, /*TIME-DATE-TEMPERATURE*/
    eAPP_STAT_RUN_TTH       = 2, /*TIME-TEMPERATURE-HUMIDITY*/
    eAPP_STAT_RUN_MIX       = 3, /*MIXED*/
    eAPP_STAT_OTA           = 4, /*OTA*/
    eAPP_STAT_INFO          = 5, /*SHOW IP, SHOW */
} AppState_t;

extern volatile enum e_AppState AppState;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sets the global application state.
 * @param newState The new state to set.
 */
void Task_SetSystemState(AppState_t newState);

/**
 * @brief Gets the current global application state.
 * @return The current AppState_t value.
 */
AppState_t Task_GetSystemState(void);

#ifdef __cplusplus
}
#endif

#endif  /// __APP_COMMON_H__