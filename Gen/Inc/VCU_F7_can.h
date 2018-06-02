#ifndef __VCU_F7_can_H
#define __VCU_F7_can_H

#include "stm32f7xx_hal.h"
#include "FreeRTOS.h"
#include "queue.h"
//Message Filtering
#define CAN_NODE_ADDRESS 2
#define CAN_NODE_MESSAGE_GROUP_1

typedef struct BMU_DTC_unpacked {
int DTC_CODE;
int DTC_Severity;
int DTC_Data;
} BMU_DTC_unpacked;
extern QueueHandle_t queueBMU_DTC;
extern volatile float TempInletRadMotorRight;	// offset: 90 scaler: 0.25
extern volatile float TempOutletRadMotorRight;	// offset: 90 scaler: 0.25
extern volatile float TempInletRadMotorLeft;	// offset: 90 scaler: 0.25
extern volatile float TempOutletRadMotorLeft;	// offset: 90 scaler: 0.25
extern volatile float ButtonEMEnabled;	// offset: 0 scaler: 1
extern volatile float ButtonHVEnabled;	// offset: 0 scaler: 1
extern volatile float StateInverterRight;	// offset: 90 scaler: 0.25
extern volatile float TempInverterDeltaRight;	// offset: 90 scaler: 0.25
extern volatile float TempInverterRight;	// offset: 90 scaler: 0.25
extern volatile float TempMotorRight;	// offset: 90 scaler: 0.25
extern volatile float TempMotorDeltaRight;	// offset: 90 scaler: 0.25
extern volatile float CurrentHVBusInverterRight;	// offset: 0 scaler: 1
extern volatile float VoltageHVBusInverterRight;	// offset: 0 scaler: 1
extern volatile float SpeedMotorRight;	// offset: 90 scaler: 0.25
extern volatile float TorqueEstimateRight;	// offset: 0 scaler: 1
extern volatile float TorqueAvailableDriveRight;	// offset: 0 scaler: 1
extern volatile float TorqueAvailableBrakingRight;	// offset: 0 scaler: 1
extern volatile float StateInverterLeft;	// offset: 90 scaler: 0.25
extern volatile float TempInverterDeltaLeft;	// offset: 90 scaler: 0.25
extern volatile float TempInverterLeft;	// offset: 90 scaler: 0.25
extern volatile float TempMotorLeft;	// offset: 90 scaler: 0.25
extern volatile float TempMotorDeltaLeft;	// offset: 90 scaler: 0.25
extern volatile float CurrentHVBusInverterLeft;	// offset: 0 scaler: 1
extern volatile float VoltageHVBusInverterLeft;	// offset: 0 scaler: 1
extern volatile float SpeedMotorLeft;	// offset: 90 scaler: 0.25
extern volatile float TorqueEstimateLeft;	// offset: 0 scaler: 1
extern volatile float TorqueAvailableDriveLeft;	// offset: 0 scaler: 1
extern volatile float TorqueAvailableBrakingLeft;	// offset: 0 scaler: 1
extern volatile float SpeedWheelRightBack;	// offset: 60 scaler: 0.000686648127167
extern volatile float SpeedWheelLeftBack;	// offset: 60 scaler: 0.000686648127167
extern volatile float VoltageBatteryLV;	// offset: 0 scaler: 1
extern volatile float VoltageBatteryHV;	// offset: 0 scaler: 1
extern volatile float CurrentDCBatteryHV;	// offset: 0 scaler: 0.001
extern volatile float PowerBatteryHV;	// offset: 0 scaler: 0.01
extern volatile float StateBatteryChargeHV;	// offset: 0 scaler: 0.1
extern volatile float StateBatteryHealthHV;	// offset: 0 scaler: 0.1
extern volatile float StateBatteryPowerHV;	// offset: 0 scaler: 0.1
extern volatile float StateBMS;	// offset: 0 scaler: 1
extern volatile float VoltageBusHV;	// offset: 0 scaler: 0.1
extern volatile float SpeedWheelLeftFront;	// offset: 60 scaler: 0.000686648127167
extern volatile float SpeedWheelRightFront;	// offset: 60 scaler: 0.000686648127167
extern volatile float DTC_CODE;	// offset: 0 scaler: 1
extern volatile float DTC_Severity;	// offset: 0 scaler: 1
extern volatile float DTC_Data;	// offset: 0 scaler: 1
extern volatile float SpeedVehicleVCU;	// offset: 60 scaler: 0.000686648127167
extern volatile float VoltageLimitHighInverterRight;	// offset: 0 scaler: 1
extern volatile float VoltageLimitLowInverterRight;	// offset: 0 scaler: 1
extern volatile float CurrentLimitDschrgInverterRight;	// offset: 0 scaler: 1
extern volatile float CurrentLimitChargeInverterRight;	// offset: 0 scaler: 1
extern volatile float InverterCommandRight;	// offset: 0 scaler: 1
extern volatile float SpeedLimitForwardRight;	// offset: 0 scaler: 1
extern volatile float SpeedLimitReverseRight;	// offset: 0 scaler: 1
extern volatile float TorqueDemandRight;	// offset: 0 scaler: 1
extern volatile float TorqueLimitBrakingRight;	// offset: 0 scaler: 1
extern volatile float TorqueLimitDriveRight;	// offset: 0 scaler: 1
extern volatile float VoltageLimitHighInverterLeft;	// offset: 0 scaler: 1
extern volatile float VoltageLimitLowInverterLeft;	// offset: 0 scaler: 1
extern volatile float CurrentLimitDschrgInverterLeft;	// offset: 0 scaler: 1
extern volatile float CurrentLimitChargeInverterLeft;	// offset: 0 scaler: 1
extern volatile float InverterCommandLeft;	// offset: 0 scaler: 1
extern volatile float SpeedLimitForwardLeft;	// offset: 0 scaler: 1
extern volatile float SpeedLimitReverseLeft;	// offset: 0 scaler: 1
extern volatile float TorqueDemandLeft;	// offset: 0 scaler: 1
extern volatile float TorqueLimitBrakingLeft;	// offset: 0 scaler: 1
extern volatile float TorqueLimitDriveLeft;	// offset: 0 scaler: 1
extern int VoltageLimitRight_PRO_CAN_SEED;
extern int VoltageLimitRight_PRO_CAN_COUNT;
extern int CurrentLimitRight_PRO_CAN_SEED;
extern int CurrentLimitRight_PRO_CAN_COUNT;
extern int SpeedLimitRight_PRO_CAN_SEED;
extern int SpeedLimitRight_PRO_CAN_COUNT;
extern int TorqueLimitRight_PRO_CAN_SEED;
extern int TorqueLimitRight_PRO_CAN_COUNT;
extern int VoltageLimitLeft_PRO_CAN_SEED;
extern int VoltageLimitLeft_PRO_CAN_COUNT;
extern int CurrentLimitLeft_PRO_CAN_SEED;
extern int CurrentLimitLeft_PRO_CAN_COUNT;
extern int SpeedLimitLeft_PRO_CAN_SEED;
extern int SpeedLimitLeft_PRO_CAN_COUNT;
extern int TorqueLimitLeft_PRO_CAN_SEED;
extern int TorqueLimitLeft_PRO_CAN_COUNT;
int init_can_driver();
int parseCANData(int id, void * data);
int sendCAN_VCU_F7_DTC();
int sendCAN_VCU_Status();
int sendCAN_VCU_VERSION();
int sendCAN_VoltageLimitRight();
int sendCAN_CurrentLimitRight();
int sendCAN_SpeedLimitRight();
int sendCAN_TorqueLimitRight();
int sendCAN_VoltageLimitLeft();
int sendCAN_CurrentLimitLeft();
int sendCAN_SpeedLimitLeft();
int sendCAN_TorqueLimitLeft();
void configCANFilters(CAN_HandleTypeDef* canHandle);
#endif /*__VCU_F7_can_H */
