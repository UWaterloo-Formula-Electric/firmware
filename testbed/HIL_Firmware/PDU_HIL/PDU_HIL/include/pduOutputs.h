#ifndef PDU_OUTPUTS_H
#define PDU_OUTPUTS_H

#define RELAY_PDU_OUTPUT_INTERVAL_MS 100

#define POW_AUX_PIN 7
#define POW_BRAKE_LIGHT_PIN 15
#define BATTERY_RAW_PIN 16
#define POW_BMU_PIN 46
#define POW_VCU_PIN 9
#define POW_DCU_PIN 10
#define POW_MC_LEFT_PIN 33
#define POW_MC_RIGHT_PIN 34
#define POW_LEFT_PUMP_PIN 35
#define POW_RIGHT_PUMP_PIN 36
#define POW_LEFT_FAN_PIN 37
#define POW_RIGHT_FAN_PIN 38

#define RELAY_PDU_OUTPUTS_CAN_ID 0x8030F03
#define EXTENDED_MSG 1
#define PDU_OUTPUT_CAN_MSG_DATA_SIZE 2 //size in bytes

void relayPduOutputs(void * pvParameters);

typedef enum PduOutStatusBits_E{
    PduOutStatusBit_Aux = 0,
    PduOutStatusBit_BrakeLight,
    PduOutStatusBit_Battery,
    PduOutStatusBit_Bmu,
    PduOutStatusBit_Vcu,
    PduOutStatusBit_Dcu,
    PduOutStatusBit_McLeft,
    PduOutStatusBit_McRight,
    PduOutStatusBit_LeftPump,
    PduOutStatusBit_RightPump,
    PduOutStatusBit_LeftFan,
    PduOutStatusBit_RightFan,
}PduOutStatusBits_E;

#endif/*PDU_OUTPUTS_H*/
