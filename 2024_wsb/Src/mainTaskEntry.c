#include "FreeRTOS.h"
#include "bsp.h"
#include "debug.h"
#include "main.h"
#include "task.h"

#define MAIN_TASK_PERIOD 1000

void mainTaskFunction(void const* argument) {
#if BOARD_ID == ID_WSBFL
    char* boardName = "WSBFL";
#elif BOARD_ID == ID_WSBFR
    char* boardName = "WSBFR";
#elif BOARD_ID == ID_WSBRL
    char* boardName = "WSBRL";
#elif BOARD_ID == ID_WSBRR
    char* boardName = "WSBRR";
#endif
    DEBUG_PRINT("Starting up %s!!\n", boardName);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        HAL_GPIO_TogglePin(FW_HEARTBEAT_GPIO_Port, FW_HEARTBEAT_Pin);
        vTaskDelayUntil(&xLastWakeTime, MAIN_TASK_PERIOD);
    }
}
