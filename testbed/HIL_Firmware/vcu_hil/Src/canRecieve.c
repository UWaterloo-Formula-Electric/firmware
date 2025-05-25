#include "button.h"
#include "vcu_hil_can.h"
#include "userCanF7.h"
#include "bsp.h"
#include "canRecieve.h"
#include "Dac_Driver.h"
#include "userCan.h"
#include "main.h"


// these values are all in mV
void CAN_Msg_SteeringRaw_Callback()
{
    SteeringRaw *= 1000;
    DAC_SendSignal(&hi2c1, dac1, SteeringRaw)
}

void CAN_Msg_BrakePosition_Callback()
{
    BrakePosition *= 1000;
    DAC_SendSignal(&hi2c1, dac1, BrakePosition)
}

void CAN_Msg_BrakePresRaw_Callback()
{
    BrakePresRaw *= 1000;
    DAC_SendSignal(&hi2c1, dac1, BrakePresRaw)
}

void CAN_Msg_ThrottlePositionA_Callback()
{
    ThrottlePositionA *= 1000;
    onboard_DAC_Start(DAC1_CHANNEL_1, ThrottlePositionA);
    
}
void CAN_Msg_ThrottlePositionB_Callback()
{
    ThrottlePositionB *= 1000;
    onboard_DAC_Start(DAC1_CHANNEL_2, ThrottlePositionB);
}

void CAN_Msg_VCU_Button_Input_Callback()
{
    // active low buttons
    HAL_GPIO_WritePin(HV_BTN_GPIO_Port, HV_BTN_Pin, HV_BTN);
    HAL_GPIO_WritePin(EM_BTN_GPIO_Port, EM_BTN_Pin, EM_BTN);
    HAL_GPIO_WritePin(TC_BTN_GPIO_Port, TC_BTN_Pin, TC_BTN);
    HAL_GPIO_WritePin(ENDUR_GPIO_Port, ENDUR_Pin, ENDUR);
}
