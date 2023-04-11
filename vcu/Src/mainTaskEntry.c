#include "FreeRTOS.h"
#include "task.h"

#include "bsp.h"
#include "debug.h"
#include "vcu_F7_can.h"
#include "canReceive.h"
#include "motorController.h"
#include "traction_control.h"
#include "endurance_mode.h"

#define MAIN_TASK_PERIOD 200
#define HEARTBEAT_LED_TOGGLE_PERIOD 1000

void WiCANPublish(void)
{
    static VCU_CAN_CONFIGURED_E vcuCANConfiguredIndex = VCU_CAN_CONFIGURED_INITIAL_SOC;
    WiCANFeedbackVCUEnum = vcuCANConfiguredIndex;
    switch (vcuCANConfiguredIndex)
    {
        case VCU_CAN_CONFIGURED_INITIAL_SOC:
            WiCANFeedbackVCUValue = get_initial_soc() / WIRELESS_CAN_FLOAT_SCALAR;
            sendCAN_WiCAN_FeedbackVCU();
            break;
        
        case VCU_CAN_CONFIGURED_NUM_LAPS:
            WiCANFeedbackVCUValue = get_num_laps();
            sendCAN_WiCAN_FeedbackVCU();
            break;

        case VCU_CAN_CONFIGURED_NUM_LAPS_TO_COMPLETE:
            WiCANFeedbackVCUValue = get_num_laps_complete();
            sendCAN_WiCAN_FeedbackVCU();
            break;

        case VCU_CAN_CONFIGURED_IN_ENDURANCE_MODE:
            WiCANFeedbackVCUValue = get_in_endurance_mode();
            sendCAN_WiCAN_FeedbackVCU();
            break;

        case VCU_CAN_CONFIGURED_EM_KP:
            WiCANFeedbackVCUValue = get_em_kP() / WIRELESS_CAN_FLOAT_SCALAR;
            sendCAN_WiCAN_FeedbackVCU();
            break;

        case VCU_CAN_CONFIGURED_EM_KI:
            WiCANFeedbackVCUValue = get_em_kI() / WIRELESS_CAN_FLOAT_SCALAR;
            sendCAN_WiCAN_FeedbackVCU();
            break;

        case VCU_CAN_CONFIGURED_TC_ON:
            WiCANFeedbackVCUValue = get_tc();
            sendCAN_WiCAN_FeedbackVCU();
            break;

        case VCU_CAN_CONFIGURED_TC_KP:
            WiCANFeedbackVCUValue = get_tc_kP() / WIRELESS_CAN_FLOAT_SCALAR;
            sendCAN_WiCAN_FeedbackVCU();
            break;

        case VCU_CAN_CONFIGURED_TC_ERROR_FLOOR_RAD_S:
            WiCANFeedbackVCUValue = get_tc_error_floor_rad_s() / WIRELESS_CAN_FLOAT_SCALAR;
            sendCAN_WiCAN_FeedbackVCU();
            break;

        case VCU_CAN_CONFIGURED_TC_MIN_PERCENT_ERROR:
            WiCANFeedbackVCUValue = get_tc_min_percent_error() / WIRELESS_CAN_FLOAT_SCALAR;
            sendCAN_WiCAN_FeedbackVCU();
            break;

        case VCU_CAN_CONFIGURED_TC_TORQUE_MAX_FLOOR:
            WiCANFeedbackVCUValue = get_tc_torque_max_floor() / WIRELESS_CAN_FLOAT_SCALAR;
            sendCAN_WiCAN_FeedbackVCU();
            break;

        case VCU_CAN_CONFIGURED_TV_DEAD_ZONE_END_RIGHT:
            WiCANFeedbackVCUValue = get_tv_deadzone_end_right() / WIRELESS_CAN_FLOAT_SCALAR;
            sendCAN_WiCAN_FeedbackVCU();
            break;

        case VCU_CAN_CONFIGURED_TV_DEAD_ZONE_END_LEFT:
            WiCANFeedbackVCUValue = get_tv_deadzone_end_left() / WIRELESS_CAN_FLOAT_SCALAR;
            sendCAN_WiCAN_FeedbackVCU();
            break;

        case VCU_CAN_CONFIGURED_TORQUE_VECTOR_FACTOR:
            WiCANFeedbackVCUValue = get_torque_vector_factor() / WIRELESS_CAN_FLOAT_SCALAR;
            sendCAN_WiCAN_FeedbackVCU();
            break;

        case VCU_CAN_CONFIGURED_MAX_TORQUE_DEMAND:
            WiCANFeedbackVCUValue = get_max_torque_demand() / WIRELESS_CAN_FLOAT_SCALAR;
            sendCAN_WiCAN_FeedbackVCU();
            break;

        case VCU_CAN_CONFIGURED_COUNT:
        default:
            break;

    }
    vcuCANConfiguredIndex = (vcuCANConfiguredIndex + 1) % VCU_CAN_CONFIGURED_COUNT;
}

void mainTaskFunction(void const * argument)
{
    DEBUG_PRINT("Starting up!!\n");
    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xLastLEDToggle = xTaskGetTickCount();

    while (1)
    {
        if (xLastLEDToggle - xTaskGetTickCount() >= HEARTBEAT_LED_TOGGLE_PERIOD)
        {
            HAL_GPIO_TogglePin(DEBUG_LED_PORT, DEBUG_LED_PIN);
            xLastLEDToggle = xTaskGetTickCount();
        }

        WiCANPublish();

        vTaskDelayUntil(&xLastWakeTime, MAIN_TASK_PERIOD);
    }
}