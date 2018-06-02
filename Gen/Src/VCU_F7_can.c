//DBC version:
int DBCVersion = 1;
char gitCommit[] = "0df2a9a";
//VCU_F7 can headder
#include "VCU_F7_can.h"
#include "CRC_CALC.h"
#include "userCan.h"
// Incoming variables
QueueHandle_t queueBMU_DTC;
int DTC_CODEReceived(int64_t newValue)
{
	newValue = newValue * 1;
	newValue = newValue + 0;
	return newValue;
}

int DTC_SeverityReceived(int64_t newValue)
{
	newValue = newValue * 1;
	newValue = newValue + 0;
	return newValue;
}

int DTC_DataReceived(int64_t newValue)
{
	newValue = newValue * 1;
	newValue = newValue + 0;
	return newValue;
}

volatile float TempInletRadMotorRight;	// offset: 90 scaler: 0.25
void TempInletRadMotorRightReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.25;
	floatValue = floatValue + 90;
	TempInletRadMotorRight = floatValue;
}

volatile float TempOutletRadMotorRight;	// offset: 90 scaler: 0.25
void TempOutletRadMotorRightReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.25;
	floatValue = floatValue + 90;
	TempOutletRadMotorRight = floatValue;
}

volatile float TempInletRadMotorLeft;	// offset: 90 scaler: 0.25
void TempInletRadMotorLeftReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.25;
	floatValue = floatValue + 90;
	TempInletRadMotorLeft = floatValue;
}

volatile float TempOutletRadMotorLeft;	// offset: 90 scaler: 0.25
void TempOutletRadMotorLeftReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.25;
	floatValue = floatValue + 90;
	TempOutletRadMotorLeft = floatValue;
}

volatile float ButtonEMEnabled;	// offset: 0 scaler: 1
void ButtonEMEnabledReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 1;
	floatValue = floatValue + 0;
	ButtonEMEnabled = floatValue;
}

volatile float ButtonHVEnabled;	// offset: 0 scaler: 1
void ButtonHVEnabledReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 1;
	floatValue = floatValue + 0;
	ButtonHVEnabled = floatValue;
}

volatile float StateInverterRight;	// offset: 90 scaler: 0.25
void StateInverterRightReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.25;
	floatValue = floatValue + 90;
	StateInverterRight = floatValue;
}

volatile float TempInverterDeltaRight;	// offset: 90 scaler: 0.25
void TempInverterDeltaRightReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.25;
	floatValue = floatValue + 90;
	TempInverterDeltaRight = floatValue;
}

volatile float TempInverterRight;	// offset: 90 scaler: 0.25
void TempInverterRightReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.25;
	floatValue = floatValue + 90;
	TempInverterRight = floatValue;
}

volatile float TempMotorRight;	// offset: 90 scaler: 0.25
void TempMotorRightReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.25;
	floatValue = floatValue + 90;
	TempMotorRight = floatValue;
}

volatile float TempMotorDeltaRight;	// offset: 90 scaler: 0.25
void TempMotorDeltaRightReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.25;
	floatValue = floatValue + 90;
	TempMotorDeltaRight = floatValue;
}

volatile float CurrentHVBusInverterRight;	// offset: 0 scaler: 1
void CurrentHVBusInverterRightReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 1;
	floatValue = floatValue + 0;
	CurrentHVBusInverterRight = floatValue;
}

volatile float VoltageHVBusInverterRight;	// offset: 0 scaler: 1
void VoltageHVBusInverterRightReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 1;
	floatValue = floatValue + 0;
	VoltageHVBusInverterRight = floatValue;
}

volatile float SpeedMotorRight;	// offset: 90 scaler: 0.25
void SpeedMotorRightReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.25;
	floatValue = floatValue + 90;
	SpeedMotorRight = floatValue;
}

volatile float TorqueEstimateRight;	// offset: 0 scaler: 1
void TorqueEstimateRightReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 1;
	floatValue = floatValue + 0;
	TorqueEstimateRight = floatValue;
}

volatile float TorqueAvailableDriveRight;	// offset: 0 scaler: 1
void TorqueAvailableDriveRightReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 1;
	floatValue = floatValue + 0;
	TorqueAvailableDriveRight = floatValue;
}

volatile float TorqueAvailableBrakingRight;	// offset: 0 scaler: 1
void TorqueAvailableBrakingRightReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 1;
	floatValue = floatValue + 0;
	TorqueAvailableBrakingRight = floatValue;
}

volatile float StateInverterLeft;	// offset: 90 scaler: 0.25
void StateInverterLeftReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.25;
	floatValue = floatValue + 90;
	StateInverterLeft = floatValue;
}

volatile float TempInverterDeltaLeft;	// offset: 90 scaler: 0.25
void TempInverterDeltaLeftReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.25;
	floatValue = floatValue + 90;
	TempInverterDeltaLeft = floatValue;
}

volatile float TempInverterLeft;	// offset: 90 scaler: 0.25
void TempInverterLeftReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.25;
	floatValue = floatValue + 90;
	TempInverterLeft = floatValue;
}

volatile float TempMotorLeft;	// offset: 90 scaler: 0.25
void TempMotorLeftReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.25;
	floatValue = floatValue + 90;
	TempMotorLeft = floatValue;
}

volatile float TempMotorDeltaLeft;	// offset: 90 scaler: 0.25
void TempMotorDeltaLeftReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.25;
	floatValue = floatValue + 90;
	TempMotorDeltaLeft = floatValue;
}

volatile float CurrentHVBusInverterLeft;	// offset: 0 scaler: 1
void CurrentHVBusInverterLeftReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 1;
	floatValue = floatValue + 0;
	CurrentHVBusInverterLeft = floatValue;
}

volatile float VoltageHVBusInverterLeft;	// offset: 0 scaler: 1
void VoltageHVBusInverterLeftReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 1;
	floatValue = floatValue + 0;
	VoltageHVBusInverterLeft = floatValue;
}

volatile float SpeedMotorLeft;	// offset: 90 scaler: 0.25
void SpeedMotorLeftReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.25;
	floatValue = floatValue + 90;
	SpeedMotorLeft = floatValue;
}

volatile float TorqueEstimateLeft;	// offset: 0 scaler: 1
void TorqueEstimateLeftReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 1;
	floatValue = floatValue + 0;
	TorqueEstimateLeft = floatValue;
}

volatile float TorqueAvailableDriveLeft;	// offset: 0 scaler: 1
void TorqueAvailableDriveLeftReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 1;
	floatValue = floatValue + 0;
	TorqueAvailableDriveLeft = floatValue;
}

volatile float TorqueAvailableBrakingLeft;	// offset: 0 scaler: 1
void TorqueAvailableBrakingLeftReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 1;
	floatValue = floatValue + 0;
	TorqueAvailableBrakingLeft = floatValue;
}

volatile float SpeedWheelRightBack;	// offset: 60 scaler: 0.000686648127167
void SpeedWheelRightBackReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.000686648127167;
	floatValue = floatValue + 60;
	SpeedWheelRightBack = floatValue;
}

volatile float SpeedWheelLeftBack;	// offset: 60 scaler: 0.000686648127167
void SpeedWheelLeftBackReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.000686648127167;
	floatValue = floatValue + 60;
	SpeedWheelLeftBack = floatValue;
}

volatile float VoltageBatteryLV;	// offset: 0 scaler: 1
void VoltageBatteryLVReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 1;
	floatValue = floatValue + 0;
	VoltageBatteryLV = floatValue;
}

volatile float VoltageBatteryHV;	// offset: 0 scaler: 1
void VoltageBatteryHVReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 1;
	floatValue = floatValue + 0;
	VoltageBatteryHV = floatValue;
}

volatile float CurrentDCBatteryHV;	// offset: 0 scaler: 0.001
void CurrentDCBatteryHVReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.001;
	floatValue = floatValue + 0;
	CurrentDCBatteryHV = floatValue;
}

volatile float PowerBatteryHV;	// offset: 0 scaler: 0.01
void PowerBatteryHVReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.01;
	floatValue = floatValue + 0;
	PowerBatteryHV = floatValue;
}

volatile float StateBatteryChargeHV;	// offset: 0 scaler: 0.1
void StateBatteryChargeHVReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.1;
	floatValue = floatValue + 0;
	StateBatteryChargeHV = floatValue;
}

volatile float StateBatteryHealthHV;	// offset: 0 scaler: 0.1
void StateBatteryHealthHVReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.1;
	floatValue = floatValue + 0;
	StateBatteryHealthHV = floatValue;
}

volatile float StateBatteryPowerHV;	// offset: 0 scaler: 0.1
void StateBatteryPowerHVReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.1;
	floatValue = floatValue + 0;
	StateBatteryPowerHV = floatValue;
}

volatile float StateBMS;	// offset: 0 scaler: 1
void StateBMSReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 1;
	floatValue = floatValue + 0;
	StateBMS = floatValue;
}

volatile float VoltageBusHV;	// offset: 0 scaler: 0.1
void VoltageBusHVReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.1;
	floatValue = floatValue + 0;
	VoltageBusHV = floatValue;
}

volatile float SpeedWheelLeftFront;	// offset: 60 scaler: 0.000686648127167
void SpeedWheelLeftFrontReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.000686648127167;
	floatValue = floatValue + 60;
	SpeedWheelLeftFront = floatValue;
}

volatile float SpeedWheelRightFront;	// offset: 60 scaler: 0.000686648127167
void SpeedWheelRightFrontReceived(int64_t newValue)
{
	float floatValue = (float)newValue * 0.000686648127167;
	floatValue = floatValue + 60;
	SpeedWheelRightFront = floatValue;
}

// Outgoing variables
volatile float DTC_CODE;	// offset: 0 scaler: 1
__weak int64_t DTC_CODESending()
{
	float sendValue = DTC_CODE;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float DTC_Severity;	// offset: 0 scaler: 1
__weak int64_t DTC_SeveritySending()
{
	float sendValue = DTC_Severity;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float DTC_Data;	// offset: 0 scaler: 1
__weak int64_t DTC_DataSending()
{
	float sendValue = DTC_Data;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float SpeedVehicleVCU;	// offset: 60 scaler: 0.000686648127167
__weak int64_t SpeedVehicleVCUSending()
{
	float sendValue = SpeedVehicleVCU;
	sendValue = sendValue - 60;
	sendValue = sendValue / 0.000686648127167;
	return sendValue;
}

volatile float VoltageLimitHighInverterRight;	// offset: 0 scaler: 1
__weak int64_t VoltageLimitHighInverterRightSending()
{
	float sendValue = VoltageLimitHighInverterRight;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float VoltageLimitLowInverterRight;	// offset: 0 scaler: 1
__weak int64_t VoltageLimitLowInverterRightSending()
{
	float sendValue = VoltageLimitLowInverterRight;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float CurrentLimitDschrgInverterRight;	// offset: 0 scaler: 1
__weak int64_t CurrentLimitDschrgInverterRightSending()
{
	float sendValue = CurrentLimitDschrgInverterRight;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float CurrentLimitChargeInverterRight;	// offset: 0 scaler: 1
__weak int64_t CurrentLimitChargeInverterRightSending()
{
	float sendValue = CurrentLimitChargeInverterRight;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float InverterCommandRight;	// offset: 0 scaler: 1
__weak int64_t InverterCommandRightSending()
{
	float sendValue = InverterCommandRight;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float SpeedLimitForwardRight;	// offset: 0 scaler: 1
__weak int64_t SpeedLimitForwardRightSending()
{
	float sendValue = SpeedLimitForwardRight;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float SpeedLimitReverseRight;	// offset: 0 scaler: 1
__weak int64_t SpeedLimitReverseRightSending()
{
	float sendValue = SpeedLimitReverseRight;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float TorqueDemandRight;	// offset: 0 scaler: 1
__weak int64_t TorqueDemandRightSending()
{
	float sendValue = TorqueDemandRight;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float TorqueLimitBrakingRight;	// offset: 0 scaler: 1
__weak int64_t TorqueLimitBrakingRightSending()
{
	float sendValue = TorqueLimitBrakingRight;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float TorqueLimitDriveRight;	// offset: 0 scaler: 1
__weak int64_t TorqueLimitDriveRightSending()
{
	float sendValue = TorqueLimitDriveRight;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float VoltageLimitHighInverterLeft;	// offset: 0 scaler: 1
__weak int64_t VoltageLimitHighInverterLeftSending()
{
	float sendValue = VoltageLimitHighInverterLeft;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float VoltageLimitLowInverterLeft;	// offset: 0 scaler: 1
__weak int64_t VoltageLimitLowInverterLeftSending()
{
	float sendValue = VoltageLimitLowInverterLeft;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float CurrentLimitDschrgInverterLeft;	// offset: 0 scaler: 1
__weak int64_t CurrentLimitDschrgInverterLeftSending()
{
	float sendValue = CurrentLimitDschrgInverterLeft;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float CurrentLimitChargeInverterLeft;	// offset: 0 scaler: 1
__weak int64_t CurrentLimitChargeInverterLeftSending()
{
	float sendValue = CurrentLimitChargeInverterLeft;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float InverterCommandLeft;	// offset: 0 scaler: 1
__weak int64_t InverterCommandLeftSending()
{
	float sendValue = InverterCommandLeft;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float SpeedLimitForwardLeft;	// offset: 0 scaler: 1
__weak int64_t SpeedLimitForwardLeftSending()
{
	float sendValue = SpeedLimitForwardLeft;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float SpeedLimitReverseLeft;	// offset: 0 scaler: 1
__weak int64_t SpeedLimitReverseLeftSending()
{
	float sendValue = SpeedLimitReverseLeft;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float TorqueDemandLeft;	// offset: 0 scaler: 1
__weak int64_t TorqueDemandLeftSending()
{
	float sendValue = TorqueDemandLeft;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float TorqueLimitBrakingLeft;	// offset: 0 scaler: 1
__weak int64_t TorqueLimitBrakingLeftSending()
{
	float sendValue = TorqueLimitBrakingLeft;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}

volatile float TorqueLimitDriveLeft;	// offset: 0 scaler: 1
__weak int64_t TorqueLimitDriveLeftSending()
{
	float sendValue = TorqueLimitDriveLeft;
	sendValue = sendValue - 0;
	sendValue = sendValue / 1;
	return sendValue;
}


// PRO_CAN 
int VoltageLimitRight_PRO_CAN_SEED = 127;
int VoltageLimitRight_PRO_CAN_COUNT = 0;
int CurrentLimitRight_PRO_CAN_SEED = 127;
int CurrentLimitRight_PRO_CAN_COUNT = 0;
int SpeedLimitRight_PRO_CAN_SEED = 127;
int SpeedLimitRight_PRO_CAN_COUNT = 0;
int TorqueLimitRight_PRO_CAN_SEED = 127;
int TorqueLimitRight_PRO_CAN_COUNT = 0;
int VoltageLimitLeft_PRO_CAN_SEED = 127;
int VoltageLimitLeft_PRO_CAN_COUNT = 0;
int CurrentLimitLeft_PRO_CAN_SEED = 127;
int CurrentLimitLeft_PRO_CAN_COUNT = 0;
int SpeedLimitLeft_PRO_CAN_SEED = 127;
int SpeedLimitLeft_PRO_CAN_COUNT = 0;
int TorqueLimitLeft_PRO_CAN_SEED = 127;
int TorqueLimitLeft_PRO_CAN_COUNT = 0;

int init_can_driver(){
	queueBMU_DTC = xQueueCreate(5, sizeof(BMU_DTC_unpacked)); if (queueBMU_DTC== NULL) return HAL_ERROR;
	generate_CRC_lookup_table();
	return HAL_OK;
}

struct VCU_F7_DTC{
	uint64_t DTC_CODE : 8;
	         int64_t DTC_Severity : 8;
	         int64_t DTC_Data : 32;
};

struct VCU_Status{
	         int64_t SpeedVehicleVCU : 18;
	uint64_t FILLER_END : 46;
};

struct VCU_VERSION{
	int DBC : 8;
	char git0 : 8;
	char git1 : 8;
	char git2 : 8;
	char git3 : 8;
	char git4 : 8;
	char git5 : 8;
	char git6 : 8;
};

struct VoltageLimitRight{
	uint64_t VoltageLimitHighInverterRight : 12;
	uint64_t VoltageLimitLowInverterRight : 12;
	uint64_t FILLER_48 : 24;
	         int64_t PRO_CAN_RES : 4;
	         int64_t PRO_CAN_COUNT : 4;
	         int64_t PRO_CAN_CRC : 8;
};

struct CurrentLimitRight{
	uint64_t CurrentLimitDschrgInverterRight : 12;
	uint64_t CurrentLimitChargeInverterRight : 12;
	uint64_t FILLER_48 : 24;
	         int64_t PRO_CAN_RES : 4;
	         int64_t PRO_CAN_COUNT : 4;
	         int64_t PRO_CAN_CRC : 8;
};

struct SpeedLimitRight{
	uint64_t InverterCommandRight : 16;
	uint64_t SpeedLimitForwardRight : 16;
	uint64_t SpeedLimitReverseRight : 16;
	         int64_t PRO_CAN_RES : 4;
	         int64_t PRO_CAN_COUNT : 4;
	         int64_t PRO_CAN_CRC : 8;
};

struct TorqueLimitRight{
	uint64_t TorqueDemandRight : 16;
	uint64_t TorqueLimitBrakingRight : 16;
	uint64_t TorqueLimitDriveRight : 16;
	         int64_t PRO_CAN_RES : 4;
	         int64_t PRO_CAN_COUNT : 4;
	         int64_t PRO_CAN_CRC : 8;
};

struct VoltageLimitLeft{
	uint64_t VoltageLimitHighInverterLeft : 12;
	uint64_t VoltageLimitLowInverterLeft : 12;
	uint64_t FILLER_48 : 24;
	         int64_t PRO_CAN_RES : 4;
	         int64_t PRO_CAN_COUNT : 4;
	         int64_t PRO_CAN_CRC : 8;
};

struct CurrentLimitLeft{
	uint64_t CurrentLimitDschrgInverterLeft : 12;
	uint64_t CurrentLimitChargeInverterLeft : 12;
	uint64_t FILLER_48 : 24;
	         int64_t PRO_CAN_RES : 4;
	         int64_t PRO_CAN_COUNT : 4;
	         int64_t PRO_CAN_CRC : 8;
};

struct SpeedLimitLeft{
	uint64_t InverterCommandLeft : 16;
	uint64_t SpeedLimitForwardLeft : 16;
	uint64_t SpeedLimitReverseLeft : 16;
	         int64_t PRO_CAN_RES : 4;
	         int64_t PRO_CAN_COUNT : 4;
	         int64_t PRO_CAN_CRC : 8;
};

struct TorqueLimitLeft{
	uint64_t TorqueDemandLeft : 16;
	uint64_t TorqueLimitBrakingLeft : 16;
	uint64_t TorqueLimitDriveLeft : 16;
	         int64_t PRO_CAN_RES : 4;
	         int64_t PRO_CAN_COUNT : 4;
	         int64_t PRO_CAN_CRC : 8;
};


struct BMU_DTC {
	uint64_t DTC_CODE : 8;
		     int64_t DTC_Severity : 8;
		     int64_t DTC_Data : 32;
};

struct TempCoolantRight {
		     int64_t TempInletRadMotorRight : 10;
		     int64_t TempOutletRadMotorRight : 10;
	uint64_t FILLER_END : 44;
};

struct TempCoolantLeft {
		     int64_t TempInletRadMotorLeft : 10;
		     int64_t TempOutletRadMotorLeft : 10;
	uint64_t FILLER_END : 44;
};

struct DCU_buttonEvents {
		     int64_t ButtonEMEnabled : 1;
		     int64_t ButtonHVEnabled : 1;
	uint64_t FILLER_END : 62;
};

struct TempInverterRight {
		     int64_t StateInverterRight : 12;
		     int64_t TempInverterDeltaRight : 12;
		     int64_t TempInverterRight : 12;
		     int64_t InverterAUTHSEEDRight : 12;
		     int64_t PRO_CAN_RES : 4;
		     int64_t PRO_CAN_COUNT : 4;
		     int64_t PRO_CAN_CRC : 8;
};

struct TempMotorRight {
		     int64_t TempMotorRight : 12;
		     int64_t TempMotorDeltaRight : 12;
	uint64_t FILLER_48 : 24;
		     int64_t PRO_CAN_RES : 4;
		     int64_t PRO_CAN_COUNT : 4;
		     int64_t PRO_CAN_CRC : 8;
};

struct BusHVFeedbackRight {
	uint64_t CurrentHVBusInverterRight : 16;
	uint64_t VoltageHVBusInverterRight : 16;
	uint64_t FILLER_48 : 16;
		     int64_t PRO_CAN_RES : 4;
		     int64_t PRO_CAN_COUNT : 4;
		     int64_t PRO_CAN_CRC : 8;
};

struct SpeedFeedbackRight {
		     int64_t SpeedMotorRight : 16;
	uint64_t FILLER_48 : 32;
		     int64_t PRO_CAN_RES : 4;
		     int64_t PRO_CAN_COUNT : 4;
		     int64_t PRO_CAN_CRC : 8;
};

struct TorqueFeedbackRight {
	uint64_t TorqueEstimateRight : 16;
	uint64_t TorqueAvailableDriveRight : 16;
	uint64_t TorqueAvailableBrakingRight : 16;
		     int64_t PRO_CAN_COUNT : 4;
		     int64_t PRO_CAN_RES : 4;
		     int64_t PRO_CAN_CRC : 8;
};

struct TempInverterLeft {
		     int64_t StateInverterLeft : 12;
		     int64_t TempInverterDeltaLeft : 12;
		     int64_t TempInverterLeft : 12;
		     int64_t InverterAUTHSEEDLeft : 12;
		     int64_t PRO_CAN_RES : 4;
		     int64_t PRO_CAN_COUNT : 4;
		     int64_t PRO_CAN_CRC : 8;
};

struct TempMotorLeft {
		     int64_t TempMotorLeft : 12;
		     int64_t TempMotorDeltaLeft : 12;
	uint64_t FILLER_48 : 24;
		     int64_t PRO_CAN_RES : 4;
		     int64_t PRO_CAN_COUNT : 4;
		     int64_t PRO_CAN_CRC : 8;
};

struct BusHVFeedbackLeft {
	uint64_t CurrentHVBusInverterLeft : 16;
	uint64_t VoltageHVBusInverterLeft : 16;
	uint64_t FILLER_48 : 16;
		     int64_t PRO_CAN_RES : 4;
		     int64_t PRO_CAN_COUNT : 4;
		     int64_t PRO_CAN_CRC : 8;
};

struct SpeedFeedbackLeft {
		     int64_t SpeedMotorLeft : 16;
	uint64_t FILLER_48 : 32;
		     int64_t PRO_CAN_RES : 4;
		     int64_t PRO_CAN_COUNT : 4;
		     int64_t PRO_CAN_CRC : 8;
};

struct TorqueFeedbackLeft {
	uint64_t TorqueEstimateLeft : 16;
	uint64_t TorqueAvailableDriveLeft : 16;
	uint64_t TorqueAvailableBrakingLeft : 16;
		     int64_t PRO_CAN_COUNT : 4;
		     int64_t PRO_CAN_RES : 4;
		     int64_t PRO_CAN_CRC : 8;
};

struct WSBRR_WheelData {
		     int64_t SpeedWheelRightBack : 18;
	uint64_t StrainXRightRear : 12;
	uint64_t StrainYRightRear : 12;
	uint64_t StrainZRightRear : 12;
	uint64_t FILLER_END : 10;
};

struct WSBRL_WheelData {
		     int64_t SpeedWheelLeftBack : 18;
	uint64_t StrainXLeftRear : 12;
	uint64_t StrainYLeftRear : 12;
	uint64_t StrainZLeftRear : 12;
	uint64_t FILLER_END : 10;
};

struct PDU_StateBatteryLV {
	uint64_t VoltageBatteryLV : 14;
	uint64_t FILLER_END : 50;
};

struct BMU_stateBatteryHV {
	uint64_t VoltageBatteryHV : 20;
		     int64_t CurrentDCBatteryHV : 22;
		     int64_t PowerBatteryHV : 22;
};

struct BMU_batteryStatusHV {
	uint64_t StateBatteryChargeHV : 10;
	uint64_t StateBatteryHealthHV : 10;
	uint64_t StateBatteryPowerHV : 12;
		     int64_t TempCellMax : 10;
		     int64_t TempCellMin : 10;
		     int64_t StateBMS : 4;
	uint64_t FILLER_END : 8;
};

struct BMU_stateBusHV {
	uint64_t VoltageBusHV : 22;
	uint64_t FILLER_24 : 2;
		     int64_t StateContactorNegative : 3;
		     int64_t StateContactorPositive : 3;
	uint64_t FILLER_32 : 2;
	uint64_t VoltageCellMax : 16;
	uint64_t VoltageCellMin : 16;
};

struct WSBFL_WheelData {
		     int64_t SpeedWheelLeftFront : 18;
	uint64_t StrainXLeftFront : 12;
	uint64_t StrainYLeftFront : 12;
	uint64_t StrainZLeftFront : 12;
	uint64_t FILLER_END : 10;
};

struct WSBFR_WheelData {
		     int64_t SpeedWheelRightFront : 18;
	uint64_t StrainXRightFront : 12;
	uint64_t StrainYRightFront : 12;
	uint64_t StrainZRightFront : 12;
	uint64_t FILLER_END : 10;
};

// Message Received callbacks, declared with weak linkage to be overwritten by user functions
__weak void CAN_Msg_BMU_DTC_Callback(void)
{ return; }

__weak void CAN_Msg_TempCoolantRight_Callback(void)
{ return; }

__weak void CAN_Msg_TempCoolantLeft_Callback(void)
{ return; }

__weak void CAN_Msg_DCU_buttonEvents_Callback(void)
{ return; }

__weak void CAN_Msg_TempInverterRight_Callback(void)
{ return; }

__weak void CAN_Msg_TempMotorRight_Callback(void)
{ return; }

__weak void CAN_Msg_BusHVFeedbackRight_Callback(void)
{ return; }

__weak void CAN_Msg_SpeedFeedbackRight_Callback(void)
{ return; }

__weak void CAN_Msg_TorqueFeedbackRight_Callback(void)
{ return; }

__weak void CAN_Msg_TempInverterLeft_Callback(void)
{ return; }

__weak void CAN_Msg_TempMotorLeft_Callback(void)
{ return; }

__weak void CAN_Msg_BusHVFeedbackLeft_Callback(void)
{ return; }

__weak void CAN_Msg_SpeedFeedbackLeft_Callback(void)
{ return; }

__weak void CAN_Msg_TorqueFeedbackLeft_Callback(void)
{ return; }

__weak void CAN_Msg_WSBRR_WheelData_Callback(void)
{ return; }

__weak void CAN_Msg_WSBRL_WheelData_Callback(void)
{ return; }

__weak void CAN_Msg_PDU_StateBatteryLV_Callback(void)
{ return; }

__weak void CAN_Msg_BMU_stateBatteryHV_Callback(void)
{ return; }

__weak void CAN_Msg_BMU_batteryStatusHV_Callback(void)
{ return; }

__weak void CAN_Msg_BMU_stateBusHV_Callback(void)
{ return; }

__weak void CAN_Msg_WSBFL_WheelData_Callback(void)
{ return; }

__weak void CAN_Msg_WSBFR_WheelData_Callback(void)
{ return; }

int parseCANData(int id, void * data) {
	switch(id) {
		case 65281 : // BMU_DTC
		{
			struct BMU_DTC *in_BMU_DTC = data;
			BMU_DTC_unpacked new_BMU_DTC;
			new_BMU_DTC.DTC_CODE = DTC_CODEReceived(in_BMU_DTC->DTC_CODE);
			new_BMU_DTC.DTC_Severity = DTC_SeverityReceived(in_BMU_DTC->DTC_Severity);
			new_BMU_DTC.DTC_Data = DTC_DataReceived(in_BMU_DTC->DTC_Data);
			xQueueSendFromISR(queueBMU_DTC, &new_BMU_DTC, NULL);
			CAN_Msg_BMU_DTC_Callback();
			break;
		}
		case 35 : // TempCoolantRight
		{
			struct TempCoolantRight *in_TempCoolantRight = data;
			TempInletRadMotorRightReceived(in_TempCoolantRight->TempInletRadMotorRight);
			TempOutletRadMotorRightReceived(in_TempCoolantRight->TempOutletRadMotorRight);
			CAN_Msg_TempCoolantRight_Callback();
			break;
		}
		case 34 : // TempCoolantLeft
		{
			struct TempCoolantLeft *in_TempCoolantLeft = data;
			TempInletRadMotorLeftReceived(in_TempCoolantLeft->TempInletRadMotorLeft);
			TempOutletRadMotorLeftReceived(in_TempCoolantLeft->TempOutletRadMotorLeft);
			CAN_Msg_TempCoolantLeft_Callback();
			break;
		}
		case 67178503 : // DCU_buttonEvents
		{
			struct DCU_buttonEvents *in_DCU_buttonEvents = data;
			ButtonEMEnabledReceived(in_DCU_buttonEvents->ButtonEMEnabled);
			ButtonHVEnabledReceived(in_DCU_buttonEvents->ButtonHVEnabled);
			CAN_Msg_DCU_buttonEvents_Callback();
			break;
		}
		case 28 : // TempInverterRight
		{
			struct TempInverterRight *in_TempInverterRight = data;
			StateInverterRightReceived(in_TempInverterRight->StateInverterRight);
			TempInverterDeltaRightReceived(in_TempInverterRight->TempInverterDeltaRight);
			TempInverterRightReceived(in_TempInverterRight->TempInverterRight);
			CAN_Msg_TempInverterRight_Callback();
			break;
		}
		case 27 : // TempMotorRight
		{
			struct TempMotorRight *in_TempMotorRight = data;
			TempMotorRightReceived(in_TempMotorRight->TempMotorRight);
			TempMotorDeltaRightReceived(in_TempMotorRight->TempMotorDeltaRight);
			CAN_Msg_TempMotorRight_Callback();
			break;
		}
		case 26 : // BusHVFeedbackRight
		{
			struct BusHVFeedbackRight *in_BusHVFeedbackRight = data;
			CurrentHVBusInverterRightReceived(in_BusHVFeedbackRight->CurrentHVBusInverterRight);
			VoltageHVBusInverterRightReceived(in_BusHVFeedbackRight->VoltageHVBusInverterRight);
			CAN_Msg_BusHVFeedbackRight_Callback();
			break;
		}
		case 25 : // SpeedFeedbackRight
		{
			struct SpeedFeedbackRight *in_SpeedFeedbackRight = data;
			SpeedMotorRightReceived(in_SpeedFeedbackRight->SpeedMotorRight);
			CAN_Msg_SpeedFeedbackRight_Callback();
			break;
		}
		case 24 : // TorqueFeedbackRight
		{
			struct TorqueFeedbackRight *in_TorqueFeedbackRight = data;
			TorqueEstimateRightReceived(in_TorqueFeedbackRight->TorqueEstimateRight);
			TorqueAvailableDriveRightReceived(in_TorqueFeedbackRight->TorqueAvailableDriveRight);
			TorqueAvailableBrakingRightReceived(in_TorqueFeedbackRight->TorqueAvailableBrakingRight);
			CAN_Msg_TorqueFeedbackRight_Callback();
			break;
		}
		case 19 : // TempInverterLeft
		{
			struct TempInverterLeft *in_TempInverterLeft = data;
			StateInverterLeftReceived(in_TempInverterLeft->StateInverterLeft);
			TempInverterDeltaLeftReceived(in_TempInverterLeft->TempInverterDeltaLeft);
			TempInverterLeftReceived(in_TempInverterLeft->TempInverterLeft);
			CAN_Msg_TempInverterLeft_Callback();
			break;
		}
		case 18 : // TempMotorLeft
		{
			struct TempMotorLeft *in_TempMotorLeft = data;
			TempMotorLeftReceived(in_TempMotorLeft->TempMotorLeft);
			TempMotorDeltaLeftReceived(in_TempMotorLeft->TempMotorDeltaLeft);
			CAN_Msg_TempMotorLeft_Callback();
			break;
		}
		case 17 : // BusHVFeedbackLeft
		{
			struct BusHVFeedbackLeft *in_BusHVFeedbackLeft = data;
			CurrentHVBusInverterLeftReceived(in_BusHVFeedbackLeft->CurrentHVBusInverterLeft);
			VoltageHVBusInverterLeftReceived(in_BusHVFeedbackLeft->VoltageHVBusInverterLeft);
			CAN_Msg_BusHVFeedbackLeft_Callback();
			break;
		}
		case 16 : // SpeedFeedbackLeft
		{
			struct SpeedFeedbackLeft *in_SpeedFeedbackLeft = data;
			SpeedMotorLeftReceived(in_SpeedFeedbackLeft->SpeedMotorLeft);
			CAN_Msg_SpeedFeedbackLeft_Callback();
			break;
		}
		case 15 : // TorqueFeedbackLeft
		{
			struct TorqueFeedbackLeft *in_TorqueFeedbackLeft = data;
			TorqueEstimateLeftReceived(in_TorqueFeedbackLeft->TorqueEstimateLeft);
			TorqueAvailableDriveLeftReceived(in_TorqueFeedbackLeft->TorqueAvailableDriveLeft);
			TorqueAvailableBrakingLeftReceived(in_TorqueFeedbackLeft->TorqueAvailableBrakingLeft);
			CAN_Msg_TorqueFeedbackLeft_Callback();
			break;
		}
		case 10 : // WSBRR_WheelData
		{
			struct WSBRR_WheelData *in_WSBRR_WheelData = data;
			SpeedWheelRightBackReceived(in_WSBRR_WheelData->SpeedWheelRightBack);
			CAN_Msg_WSBRR_WheelData_Callback();
			break;
		}
		case 9 : // WSBRL_WheelData
		{
			struct WSBRL_WheelData *in_WSBRL_WheelData = data;
			SpeedWheelLeftBackReceived(in_WSBRL_WheelData->SpeedWheelLeftBack);
			CAN_Msg_WSBRL_WheelData_Callback();
			break;
		}
		case 8 : // PDU_StateBatteryLV
		{
			struct PDU_StateBatteryLV *in_PDU_StateBatteryLV = data;
			VoltageBatteryLVReceived(in_PDU_StateBatteryLV->VoltageBatteryLV);
			CAN_Msg_PDU_StateBatteryLV_Callback();
			break;
		}
		case 7 : // BMU_stateBatteryHV
		{
			struct BMU_stateBatteryHV *in_BMU_stateBatteryHV = data;
			VoltageBatteryHVReceived(in_BMU_stateBatteryHV->VoltageBatteryHV);
			CurrentDCBatteryHVReceived(in_BMU_stateBatteryHV->CurrentDCBatteryHV);
			PowerBatteryHVReceived(in_BMU_stateBatteryHV->PowerBatteryHV);
			CAN_Msg_BMU_stateBatteryHV_Callback();
			break;
		}
		case 266241 : // BMU_batteryStatusHV
		{
			struct BMU_batteryStatusHV *in_BMU_batteryStatusHV = data;
			StateBatteryChargeHVReceived(in_BMU_batteryStatusHV->StateBatteryChargeHV);
			StateBatteryHealthHVReceived(in_BMU_batteryStatusHV->StateBatteryHealthHV);
			StateBatteryPowerHVReceived(in_BMU_batteryStatusHV->StateBatteryPowerHV);
			StateBMSReceived(in_BMU_batteryStatusHV->StateBMS);
			CAN_Msg_BMU_batteryStatusHV_Callback();
			break;
		}
		case 3 : // BMU_stateBusHV
		{
			struct BMU_stateBusHV *in_BMU_stateBusHV = data;
			VoltageBusHVReceived(in_BMU_stateBusHV->VoltageBusHV);
			CAN_Msg_BMU_stateBusHV_Callback();
			break;
		}
		case 1 : // WSBFL_WheelData
		{
			struct WSBFL_WheelData *in_WSBFL_WheelData = data;
			SpeedWheelLeftFrontReceived(in_WSBFL_WheelData->SpeedWheelLeftFront);
			CAN_Msg_WSBFL_WheelData_Callback();
			break;
		}
		case 0 : // WSBFR_WheelData
		{
			struct WSBFR_WheelData *in_WSBFR_WheelData = data;
			SpeedWheelRightFrontReceived(in_WSBFR_WheelData->SpeedWheelRightFront);
			CAN_Msg_WSBFR_WheelData_Callback();
			break;
		}
		default:
		{
			return -1;
		}
	}
	return(0);
}
int sendCAN_VCU_F7_DTC(){
	struct VCU_F7_DTC new_VCU_F7_DTC;
	new_VCU_F7_DTC.DTC_CODE = DTC_CODESending();
	new_VCU_F7_DTC.DTC_Severity = DTC_SeveritySending();
	new_VCU_F7_DTC.DTC_Data = DTC_DataSending();
	return sendCanMessage(65282,6,(uint8_t *) &new_VCU_F7_DTC);
}
int sendCAN_VCU_Status(){
	struct VCU_Status new_VCU_Status;
	new_VCU_Status.SpeedVehicleVCU = SpeedVehicleVCUSending();
	return sendCanMessage(36,8,(uint8_t *) &new_VCU_Status);
}
int sendCAN_VCU_VERSION(){
	struct VCU_VERSION new_VCU_VERSION;
	new_VCU_VERSION.DBC = DBCVersion;
	new_VCU_VERSION.git0 = gitCommit[0];
	new_VCU_VERSION.git1 = gitCommit[1];
	new_VCU_VERSION.git2 = gitCommit[2];
	new_VCU_VERSION.git3 = gitCommit[3];
	new_VCU_VERSION.git4 = gitCommit[4];
	new_VCU_VERSION.git5 = gitCommit[5];
	new_VCU_VERSION.git6 = gitCommit[6];
	return sendCanMessage(29,8,(uint8_t *) &new_VCU_VERSION);
}
int sendCAN_VoltageLimitRight(){
	struct VoltageLimitRight new_VoltageLimitRight;
	new_VoltageLimitRight.VoltageLimitHighInverterRight = VoltageLimitHighInverterRightSending();
	new_VoltageLimitRight.VoltageLimitLowInverterRight = VoltageLimitLowInverterRightSending();
	new_VoltageLimitRight.PRO_CAN_COUNT= VoltageLimitRight_PRO_CAN_COUNT++;
	VoltageLimitRight_PRO_CAN_COUNT = VoltageLimitRight_PRO_CAN_COUNT % 16;
	new_VoltageLimitRight.PRO_CAN_CRC= calculate_base_CRC((void *) &new_VoltageLimitRight)^VoltageLimitRight_PRO_CAN_SEED;
	return sendCanMessage(23,8,(uint8_t *) &new_VoltageLimitRight);
}
int sendCAN_CurrentLimitRight(){
	struct CurrentLimitRight new_CurrentLimitRight;
	new_CurrentLimitRight.CurrentLimitDschrgInverterRight = CurrentLimitDschrgInverterRightSending();
	new_CurrentLimitRight.CurrentLimitChargeInverterRight = CurrentLimitChargeInverterRightSending();
	new_CurrentLimitRight.PRO_CAN_COUNT= CurrentLimitRight_PRO_CAN_COUNT++;
	CurrentLimitRight_PRO_CAN_COUNT = CurrentLimitRight_PRO_CAN_COUNT % 16;
	new_CurrentLimitRight.PRO_CAN_CRC= calculate_base_CRC((void *) &new_CurrentLimitRight)^CurrentLimitRight_PRO_CAN_SEED;
	return sendCanMessage(22,8,(uint8_t *) &new_CurrentLimitRight);
}
int sendCAN_SpeedLimitRight(){
	struct SpeedLimitRight new_SpeedLimitRight;
	new_SpeedLimitRight.InverterCommandRight = InverterCommandRightSending();
	new_SpeedLimitRight.SpeedLimitForwardRight = SpeedLimitForwardRightSending();
	new_SpeedLimitRight.SpeedLimitReverseRight = SpeedLimitReverseRightSending();
	new_SpeedLimitRight.PRO_CAN_COUNT= SpeedLimitRight_PRO_CAN_COUNT++;
	SpeedLimitRight_PRO_CAN_COUNT = SpeedLimitRight_PRO_CAN_COUNT % 16;
	new_SpeedLimitRight.PRO_CAN_CRC= calculate_base_CRC((void *) &new_SpeedLimitRight)^SpeedLimitRight_PRO_CAN_SEED;
	return sendCanMessage(21,8,(uint8_t *) &new_SpeedLimitRight);
}
int sendCAN_TorqueLimitRight(){
	struct TorqueLimitRight new_TorqueLimitRight;
	new_TorqueLimitRight.TorqueDemandRight = TorqueDemandRightSending();
	new_TorqueLimitRight.TorqueLimitBrakingRight = TorqueLimitBrakingRightSending();
	new_TorqueLimitRight.TorqueLimitDriveRight = TorqueLimitDriveRightSending();
	new_TorqueLimitRight.PRO_CAN_COUNT= TorqueLimitRight_PRO_CAN_COUNT++;
	TorqueLimitRight_PRO_CAN_COUNT = TorqueLimitRight_PRO_CAN_COUNT % 16;
	new_TorqueLimitRight.PRO_CAN_CRC= calculate_base_CRC((void *) &new_TorqueLimitRight)^TorqueLimitRight_PRO_CAN_SEED;
	return sendCanMessage(20,8,(uint8_t *) &new_TorqueLimitRight);
}
int sendCAN_VoltageLimitLeft(){
	struct VoltageLimitLeft new_VoltageLimitLeft;
	new_VoltageLimitLeft.VoltageLimitHighInverterLeft = VoltageLimitHighInverterLeftSending();
	new_VoltageLimitLeft.VoltageLimitLowInverterLeft = VoltageLimitLowInverterLeftSending();
	new_VoltageLimitLeft.PRO_CAN_COUNT= VoltageLimitLeft_PRO_CAN_COUNT++;
	VoltageLimitLeft_PRO_CAN_COUNT = VoltageLimitLeft_PRO_CAN_COUNT % 16;
	new_VoltageLimitLeft.PRO_CAN_CRC= calculate_base_CRC((void *) &new_VoltageLimitLeft)^VoltageLimitLeft_PRO_CAN_SEED;
	return sendCanMessage(14,8,(uint8_t *) &new_VoltageLimitLeft);
}
int sendCAN_CurrentLimitLeft(){
	struct CurrentLimitLeft new_CurrentLimitLeft;
	new_CurrentLimitLeft.CurrentLimitDschrgInverterLeft = CurrentLimitDschrgInverterLeftSending();
	new_CurrentLimitLeft.CurrentLimitChargeInverterLeft = CurrentLimitChargeInverterLeftSending();
	new_CurrentLimitLeft.PRO_CAN_COUNT= CurrentLimitLeft_PRO_CAN_COUNT++;
	CurrentLimitLeft_PRO_CAN_COUNT = CurrentLimitLeft_PRO_CAN_COUNT % 16;
	new_CurrentLimitLeft.PRO_CAN_CRC= calculate_base_CRC((void *) &new_CurrentLimitLeft)^CurrentLimitLeft_PRO_CAN_SEED;
	return sendCanMessage(13,8,(uint8_t *) &new_CurrentLimitLeft);
}
int sendCAN_SpeedLimitLeft(){
	struct SpeedLimitLeft new_SpeedLimitLeft;
	new_SpeedLimitLeft.InverterCommandLeft = InverterCommandLeftSending();
	new_SpeedLimitLeft.SpeedLimitForwardLeft = SpeedLimitForwardLeftSending();
	new_SpeedLimitLeft.SpeedLimitReverseLeft = SpeedLimitReverseLeftSending();
	new_SpeedLimitLeft.PRO_CAN_COUNT= SpeedLimitLeft_PRO_CAN_COUNT++;
	SpeedLimitLeft_PRO_CAN_COUNT = SpeedLimitLeft_PRO_CAN_COUNT % 16;
	new_SpeedLimitLeft.PRO_CAN_CRC= calculate_base_CRC((void *) &new_SpeedLimitLeft)^SpeedLimitLeft_PRO_CAN_SEED;
	return sendCanMessage(12,8,(uint8_t *) &new_SpeedLimitLeft);
}
int sendCAN_TorqueLimitLeft(){
	struct TorqueLimitLeft new_TorqueLimitLeft;
	new_TorqueLimitLeft.TorqueDemandLeft = TorqueDemandLeftSending();
	new_TorqueLimitLeft.TorqueLimitBrakingLeft = TorqueLimitBrakingLeftSending();
	new_TorqueLimitLeft.TorqueLimitDriveLeft = TorqueLimitDriveLeftSending();
	new_TorqueLimitLeft.PRO_CAN_COUNT= TorqueLimitLeft_PRO_CAN_COUNT++;
	TorqueLimitLeft_PRO_CAN_COUNT = TorqueLimitLeft_PRO_CAN_COUNT % 16;
	new_TorqueLimitLeft.PRO_CAN_CRC= calculate_base_CRC((void *) &new_TorqueLimitLeft)^TorqueLimitLeft_PRO_CAN_SEED;
	return sendCanMessage(11,8,(uint8_t *) &new_TorqueLimitLeft);
}
__weak void configCANFilters(CAN_HandleTypeDef* canHandle)
{
	CAN_FilterConfTypeDef  sFilterConfig;
	// Filter msgs to this nodes Id to fifo 0
	uint32_t filterID = CAN_NODE_ADDRESS<<8;
	filterID = filterID << 3; // Filter ID is left aligned to 32 bits
	uint32_t filterMask = 0xFF00;
	filterMask = filterMask << 3; // Filter masks are also left aligned to 32 bits
	sFilterConfig.FilterNumber = 0;
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
	sFilterConfig.FilterIdHigh = (filterID>>16) & 0xFFFF;
	sFilterConfig.FilterIdLow = (filterID & 0xFFFF);
	sFilterConfig.FilterMaskIdHigh = (filterMask>>16) & 0xFFFF;
	sFilterConfig.FilterMaskIdLow = (filterMask & 0xFFFF);
	sFilterConfig.FilterFIFOAssignment = 0;
	sFilterConfig.FilterActivation = ENABLE;
	sFilterConfig.BankNumber = 0;

	if(HAL_CAN_ConfigFilter(canHandle, &sFilterConfig) != HAL_OK)
	{
	  Error_Handler();
	}

	// Filter msgs to the broadcast Id to fifo 0
	filterID = 0xFF<<8;
	filterID = filterID << 3; // Filter ID is left aligned to 32 bits
	filterMask = 0xFF00;
	filterMask = filterMask << 3; // Filter masks are also left aligned to 32 bits
	sFilterConfig.FilterNumber = 1;
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
	sFilterConfig.FilterIdHigh = (filterID>>16) & 0xFFFF;
	sFilterConfig.FilterIdLow = (filterID & 0xFFFF);
	sFilterConfig.FilterMaskIdHigh = (filterMask>>16) & 0xFFFF;
	sFilterConfig.FilterMaskIdLow = (filterMask & 0xFFFF);
	sFilterConfig.FilterFIFOAssignment = 0;
	sFilterConfig.FilterActivation = ENABLE;
	sFilterConfig.BankNumber = 1;

	if(HAL_CAN_ConfigFilter(canHandle, &sFilterConfig) != HAL_OK)
	{
	  Error_Handler();
	}

	// Filter msgs to the broadcast Id to fifo 0
	filterID = 1<<12;
	filterID = filterID << 3; // Filter ID is left aligned to 32 bits
	filterMask = 0xFF00;
	filterMask = filterMask << 3; // Filter masks are also left aligned to 32 bits
	sFilterConfig.FilterNumber = 2;
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
	sFilterConfig.FilterIdHigh = (filterID>>16) & 0xFFFF;
	sFilterConfig.FilterIdLow = (filterID & 0xFFFF);
	sFilterConfig.FilterMaskIdHigh = (filterMask>>16) & 0xFFFF;
	sFilterConfig.FilterMaskIdLow = (filterMask & 0xFFFF);
	sFilterConfig.FilterFIFOAssignment = 0;
	sFilterConfig.FilterActivation = ENABLE;
	sFilterConfig.BankNumber = 2;

	if(HAL_CAN_ConfigFilter(canHandle, &sFilterConfig) != HAL_OK)
	{
	  Error_Handler();
	}
}
