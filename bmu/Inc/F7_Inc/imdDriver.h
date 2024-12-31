#ifndef IMD_H
#define IMD_H

#include "bsp.h"

typedef enum {
	IMD_INITIALIZATION,
    IMD_OPERATIONAL,
    IMD_SELF_TEST,
} ImdStatus_e;

typedef struct ImdData {
    uint16_t isoRes;
    uint8_t isoStatus;
    uint8_t measurementCounter;
    uint16_t faults;
    uint8_t deviceStatus;
} ImdData_s;

typedef enum ImdFaults_e {
    DEVICE_ERROR_ACTIVE = 0,
    HV_POS_CONN_FAILURE,
    HV_NEG_CONN_FAILURE,
    EARTH_CONN_FAILURE,
    ISO_ALARM,
    ISO_WARNING,
    ISO_OUTDATED,
    UNBALANCE_ALARM,
    UNDERVOLTAGE_ALARM,
    UNSAFE_TO_START,
    EARTHLIFT_OPEN,
} ImdFaults_e;

#define ISOLATION_FAULT 1 << 4

void initImdMeasurements();
void updateImdData(ImdData_s *ImdData);
ImdData_s * getImdData();

#endif
