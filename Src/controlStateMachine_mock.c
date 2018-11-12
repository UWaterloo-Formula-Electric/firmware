#include "controlStateMachine_mock.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "string.h"
#include "state_machine.h"

/*volatile bool fakeBPSState = true;*/
/*bool hvEnable = false;*/

/*volatile int fakeBrakePressure = 100;*/
/*volatile int fakeThrottle = 0;*/

/*volatile HAL_StatusTypeDef fakeThrottleSuccess = HAL_OK;*/

/*void setFakeThrottle(int throttle) {*/
    /*DEBUG_PRINT("setting throttle %d\n", throttle);*/
    /*fakeThrottle = throttle;*/
/*}*/
/*void setFakeThrottleSuccess(HAL_StatusTypeDef state)*/
/*{*/
    /*DEBUG_PRINT("setting throttle success state %d\n", state);*/
    /*fakeThrottleSuccess = state;*/
/*}*/
/*void setFakeBrakePressure(int pressure)*/
/*{*/
    /*DEBUG_PRINT("setting brake pressure %d\n", pressure);*/
    /*if (pressure < MIN_BRAKE_PRESSURE) {*/
        /*fsmSendEventISR(&fsmHandle, EV_Brake_Pressure_Fault);*/
    /*}*/
    /*fakeBrakePressure = pressure;*/
/*}*/
/*void setFakeBPSState(bool state)*/
/*{*/
    /*DEBUG_PRINT("setting bps state %d\n", state);*/
    /*if (fakeBPSState && state != fakeBPSState) {*/
        /*fsmSendEventISR(&fsmHandle, EV_Bps_Fail);*/
    /*}*/
    /*fakeBPSState = state;*/
/*}*/
/*void setFakeDCUCanTimeout()*/
/*{*/
    /*DEBUG_PRINT("setting dcu can timeout state\n");*/
    /*fsmSendEventISR(&fsmHandle, EV_DCU_Can_Timeout);*/
/*}*/
/*void fakeEM_ToggleDCU()*/
/*{*/
    /*DEBUG_PRINT("fake emToggleFromDCU\n");*/
    /*fsmSendEventISR(&fsmHandle, EV_EM_Toggle);*/
/*}*/

/*void fakeHVStateChange(bool state)*/
/*{*/
    /*DEBUG_PRINT("setting hvEnable %d\n", state);*/
    /*if (hvEnable && state != hvEnable) {*/
        /*fsmSendEventISR(&fsmHandle, EV_Hv_Disable);*/
    /*}*/
    /*hvEnable = state;*/
/*}*/

/*// Mock functions*/
/*int getThrottle() {*/
    /*return fakeThrottle;*/
/*}*/
/*int getBrakePressure() {*/
    /*return fakeBrakePressure;*/
/*}*/
/*HAL_StatusTypeDef outputThrottle() {*/
    /*return fakeThrottleSuccess;*/
/*}*/
/*bool checkBPSState() {*/
    /*return fakeBPSState;*/
/*}*/
/*bool throttle_is_zero() {*/
    /*return (fakeThrottle==0);*/
/*}*/
/*bool getHvEnableState() {*/
    /*return hvEnable;*/
/*}*/



#define STR_EQ(a, b) (strncmp(a, b, sizeof(b) - 1) == 0)
void executeSerialCommand(char str[]) {
    DEBUG_PRINT("Serial command: %s\n", str);
    if (STR_EQ(str, "critical")) {
        fsmSendEventISR(&mainFsmHandle, MN_EV_HV_CriticalFailure);
    } else if (STR_EQ(str, "lv cuttoff")) {
        fsmSendEventISR(&mainFsmHandle, MN_EV_LV_Cuttoff);
    } else if (STR_EQ(str, "em enable")) {
        fsmSendEventISR(&motorFsmHandle, MTR_EV_EM_ENABLE);
    } else if (STR_EQ(str, "em disable")) {
        fsmSendEventISR(&motorFsmHandle, MTR_EV_EM_DISABLE);
    } else {
        DEBUG_PRINT("Unknown command\n");
    }
}


