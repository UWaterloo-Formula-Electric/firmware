#include "FreeRTOS.h"
#include "bsp.h"
#include "debug.h"
#include "detectWSB.h"
#include "main.h"
#include "task.h"

#define MAIN_TASK_PERIOD 1000

void mainTaskFunction(void const* argument) {
    DEBUG_PRINT("Starting up main task for: ");
    char boardName[10];
    if (!getWSBBoardName(boardName, 10))
        DEBUG_PRINT("Failed to get wsb board name\n");
    else
        DEBUG_PRINT("%s\n", boardName);

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        HAL_GPIO_TogglePin(FW_HEARTBEAT_GPIO_Port, FW_HEARTBEAT_Pin);
        vTaskDelayUntil(&xLastWakeTime, MAIN_TASK_PERIOD);
    }
}
