#include "encoders.h"

#include "bsp.h"
#include "debug.h"
#include "detectWSB.h"
#include "stm32f4xx_hal.h"
#include "tim.h"

#if BOARD_ID == ID_WSBFL
#include "wsbfl_can.h"
#elif BOARD_ID == ID_WSBFR
#include "wsbfr_can.h"
#endif

#define ROTARY_ENCODER_TASK_PERIOD_MS 100

// Used in callbacks.c
volatile float frontRPS = 0;

HAL_StatusTypeDef encoders_init(void) {
    if (detectWSB() & (WSBFL | WSBFR)) {
        if (HAL_TIM_Encoder_Start(&ENCODER_TIM_HANDLE, TIM_CHANNEL_ALL) != HAL_OK) {
            return HAL_ERROR;
        }
    }
    __HAL_TIM_ENABLE_IT(&ENCODER_RPS_TIM_HANDLE, TIM_IT_UPDATE);
    return HAL_TIM_Base_Start(&ENCODER_RPS_TIM_HANDLE);
}

void RotaryEncoderTask(void const* arg) {
    deleteWSBTaskIfNot(WSBFL | WSBFR);
    DEBUG_PRINT("Starting Rotary Encoder Task\n");

    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1) {
#if BOARD_ID == ID_WSBFL
        FL_Speed_RAD_S = frontRPS;
        sendCAN_WSBFL_Sensors();
#elif BOARD_ID == ID_WSBFR
        FR_Speed_RAD_S = frontRPS;
        sendCAN_WSBFR_Sensors();
#endif
        // DEBUG_PRINT("Enc: %ld, RPS: %f\n", __HAL_TIM_GET_COUNTER(&ENCODER_TIM_HANDLE), rpm);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(ROTARY_ENCODER_TASK_PERIOD_MS));
    }
}