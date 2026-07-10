#include "TaskCommon.h"

volatile AppState_t AppState = eAPP_STAT_INIT;

void Task_SetSystemState(AppState_t newState) {
    AppState = newState;
}

AppState_t Task_GetSystemState(void) {
    return AppState;
}
