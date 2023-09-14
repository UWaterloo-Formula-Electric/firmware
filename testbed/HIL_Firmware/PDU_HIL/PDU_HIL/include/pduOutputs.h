#ifndef PDU_OUTPUTS_H
#define PDU_OUTPUTS_H

#define RELAY_PDU_OUTPUT_INTERVAL 100

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

#define RELAY_PDU_OUTPUTS 0x8030F03

void relayPduOutputs(void * pvParameters);
extern void (*relayPduPtr)(void*);

typedef enum PduOutStatusBits_E{
    PduOutStatusBit_aux = 0,
    PduOutStatusBit_brake_light,
    PduOutStatusBit_battery,
    PduOutStatusBit_bmu,
    PduOutStatusBit_vcu,
    PduOutStatusBit_dcu,
    PduOutStatusBit_mc_left,
    PduOutStatusBit_mc_right,
    PduOutStatusBit_left_pump,
    PduOutStatusBit_right_pump,
    PduOutStatusBit_left_fan,
    PduOutStatusBit_right_fan,
}PduOutStatusBits_E;

#endif
