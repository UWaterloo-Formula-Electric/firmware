#ifndef IMD_H
#define IMD_H

#include "bsp.h"

typedef struct {
  uint32_t meas_freq;
  uint32_t meas_duty;
  uint8_t status;
} IMDMeasurements;

typedef enum {
	IMDSTATUS_Normal,
	IMDSTATUS_Undervoltage,
	IMDSTATUS_SST_Good,
	IMDSTATUS_SST_Bad,
	IMDSTATUS_Device_Error,
	IMDSTATUS_Fault_Earth,
	IMDSTATUS_Invalid
} IMDStatus;

HAL_StatusTypeDef begin_imd_measurement(void);

HAL_StatusTypeDef stop_imd_measurement(void);

HAL_StatusTypeDef init_imd_measurement(void);

IMDStatus return_imd_status(IMDMeasurements m);

IMDMeasurements get_imd_measurements(void);

#endif
