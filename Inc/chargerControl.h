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

HAL_StatusTypeDef sendChargerCommand(float maxVoltage, float maxCurrent, bool startCharging);
HAL_StatusTypeDef checkChargerStatus(ChargerStatus *statusOut);
HAL_StatusTypeDef chargerInit();

#endif /* end of include guard: CHARGERCONTROL_H */
