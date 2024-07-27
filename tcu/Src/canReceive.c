/**
  *****************************************************************************
  * @file    canReceive.c
  * @author  Richard Matthews
  * @brief   Module containing callback functions for receiving CAN messages
  * @details 
  * DTCs
  *****************************************************************************
  */

#include "canReceive.h"

#include "userCan.h"
#include "bsp.h"
#include "uwfe_debug.h"
#include "boardTypes.h"

// if a state machine is made for the TCU, state_machine.h will go into this header
// #include "controlStateMachine.h"
#include "state_machine.h"

#include "tcu_can.h"

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

void DTC_Fatal_Callback(BoardIDs board)
{
  ERROR_PRINT("Fatal DTC Receieved from board %lu \n", board);
}
