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

// typedef enum ImdFaults_e {

// } ImdFaults_e;

#define ISOLATION_FAULT 1 << 4

void begin_imd_measurement(void);
HAL_StatusTypeDef initImdData();
void updateImdData(ImdData_s *ImdData);
ImdData_s * getImdData();

#endif
