#include "FreeRTOS.h"
#include "bsp.h"
#include "debug.h"
#include "detectWSB.h"
#include "main.h"
#include "task.h"

#define MAIN_TASK_PERIOD 1000

void mainTaskFunction(void const* argument) {
    char boardName[20];
    getWSBBoardName(boardName, 20);
    DEBUG_PRINT("Starting up main task for: %s\n", boardName);

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        HAL_GPIO_TogglePin(FW_HEARTBEAT_GPIO_Port, FW_HEARTBEAT_Pin);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(MAIN_TASK_PERIOD));
    }
}



void WaterflowTempSensorTask(void const* arg) {
    while (1) {
        vTaskDelay(10000);
    }
}
