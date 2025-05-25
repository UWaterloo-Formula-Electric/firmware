#include "button.h"
#include "vcu_hil_can.h"
#include "userCanF7.h"
#include "bsp.h"
#include "recieveCAN.h"
#include "Dac_Driver.h"
#include "userCan.h"


void CAN_Msg_SteeringRaw_Callback()
{

}

void CAN_Msg_BrakePosition_Callback()
{

}

void CAN_Msg_BrakePresRaw_Callback()
{

}

void CAN_Msg_ThrottlePositionB_Callback()
{

}

void CAN_Msg_VCU_Button_Input_Callback()
{

}
void Recieve_CAN_Task()
{
    vcu_hil = xQueueCreate(CAN_MSG_MAX_NUMBER, sizeof(CAN_Message));
    if(x)

}