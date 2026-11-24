#ifndef CHARGERCONTROL_H

#define CHARGERCONTROL_H

#include "bsp.h"

#define CHARGER_OK 0
#define CHARGER_FAIL 1

typedef struct ChargerStatus {
    float current; // Amps
    float voltage; // Volts
    bool  HWFail;
    bool  OverTemp;
    bool  InputVoltageStatus;
    bool  StartingStatus;
    bool  CommunicationState;
    bool  OverallState;
} ChargerStatus;

HAL_StatusTypeDef startChargerCommunication(float maxVoltage, float maxCurrent, uint32_t watchdogTaskId);
HAL_StatusTypeDef sendChargerCommand(float maxVoltage, float maxCurrent, bool startCharging);
HAL_StatusTypeDef checkChargerStatus(ChargerStatus *statusOut);
HAL_StatusTypeDef chargerInit();

// True once at least one ChargeStatus frame has ever been parsed.
bool chargerStatusEverReceived(void);
// Milliseconds since the last parsed ChargeStatus frame (UINT32_MAX if none).
uint32_t chargerStatusAgeMs(void);

#endif /* end of include guard: CHARGERCONTROL_H */
