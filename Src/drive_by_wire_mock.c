#include "drive_by_wire_mock.h"
#include "drive_by_wire.h"
#include "debug.h"
#include "string.h"
#include "state_machine.h"

volatile bool fakeBPSState = true;
bool hvEnable = false;

volatile int fakeBrakePressure = 100;
volatile int fakeThrottle = 0;

volatile HAL_StatusTypeDef fakeThrottleSuccess = HAL_OK;

void setFakeThrottle(int throttle) {
    DEBUG_PRINT("setting throttle %d\n", throttle);
    fakeThrottle = throttle;
}
void setFakeThrottleSuccess(HAL_StatusTypeDef state)
{
    DEBUG_PRINT("setting throttle success state %d\n", state);
    fakeThrottleSuccess = state;
}
void setFakeBrakePressure(int pressure)
{
    DEBUG_PRINT("setting brake pressure %d\n", pressure);
    if (pressure < MIN_BRAKE_PRESSURE) {
        fsmSendEventISR(&fsmHandle, EV_Brake_Pressure_Fault);
    }
    fakeBrakePressure = pressure;
}
void setFakeBPSState(bool state)
{
    DEBUG_PRINT("setting bps state %d\n", state);
    if (fakeBPSState && state != fakeBPSState) {
        fsmSendEventISR(&fsmHandle, EV_Bps_Fail);
    }
    fakeBPSState = state;
}
void setFakeDCUCanTimeout()
{
    DEBUG_PRINT("setting dcu can timeout state\n");
    fsmSendEventISR(&fsmHandle, EV_DCU_Can_Timeout);
}
void fakeEM_ToggleDCU()
{
    DEBUG_PRINT("fake emToggleFromDCU\n");
    fsmSendEventISR(&fsmHandle, EV_EM_Toggle);
}

void fakeHVStateChange(bool state)
{
    DEBUG_PRINT("setting hvEnable %d\n", state);
    if (hvEnable && state != hvEnable) {
        fsmSendEventISR(&fsmHandle, EV_Hv_Disable);
    }
    hvEnable = state;
}

// Mock functions
int getThrottle() {
    return fakeThrottle;
}
int getBrakePressure() {
    return fakeBrakePressure;
}
HAL_StatusTypeDef outputThrottle() {
    return fakeThrottleSuccess;
}
bool checkBPSState() {
    return fakeBPSState;
}
bool throttle_is_zero() {
    return (fakeThrottle==0);
}
bool getHvEnableState() {
    return hvEnable;
}



#define STR_EQ(a, b) (strncmp(a, b, sizeof(b) - 1) == 0)
void executeSerialCommand(char str[]) {
    DEBUG_PRINT("Serial command: %s\n", str);
    if (STR_EQ(str, "bps fail")) {
        setFakeBPSState(false);
    } else if (STR_EQ(str, "bps ok")) {
        setFakeBPSState(true);
    } else if (STR_EQ(str, "em toggle")) {
        fakeEM_ToggleDCU();
    } else if (STR_EQ(str, "hv enable")) {
        fakeHVStateChange(true);
    } else if (STR_EQ(str, "hv disable")) {
        fakeHVStateChange(false);
    } else if (STR_EQ(str, "dcu timeout")) {
        setFakeDCUCanTimeout();
    } else if (STR_EQ(str, "throttle")) {
        sscanf(str, "throttle %u", &fakeThrottle);
        DEBUG_PRINT("setting throttle %d\n", fakeThrottle);
    } else if (STR_EQ(str, "brake pressure")) {
        int newBrakePressure = 0;
        sscanf(str, "brake pressure %u", &newBrakePressure);
        setFakeBrakePressure(newBrakePressure);
    } else if (STR_EQ(str, "state throttle fail")) {
        setFakeThrottleSuccess(false);
    } else if (STR_EQ(str, "state throttle ok")) {
        setFakeThrottleSuccess(true);
    } else if (STR_EQ(str, "print")) {
        DEBUG_PRINT("hvEnable %d\n", hvEnable);
        DEBUG_PRINT("fakeBPSState %d\n", fakeBPSState);
        DEBUG_PRINT("fakeThrottle %d\n", fakeThrottle);
        DEBUG_PRINT("fakeBrakePressure %d\n", fakeBrakePressure);
        DEBUG_PRINT("fakeThrottleSuccess %d\n", fakeThrottleSuccess);
        DEBUG_PRINT("State %lu\n", fsmGetState(&fsmHandle));
    } else {
        DEBUG_PRINT("Unknown command\n");
    }
}


