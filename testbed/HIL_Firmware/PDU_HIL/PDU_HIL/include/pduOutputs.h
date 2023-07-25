#ifndef PDU_OUTPUTS_H
#define PDU_OUTPUTS_H

#define RELAY_PDU_OUTPUT_INTERVAL 100

#define POW_AUX 7
#define POW_BRAKE_LIGHT 15
#define BATTERY_RAW 16
#define POW_BMU 46
#define POW_VCU 9
#define POW_DCU 10
#define POW_MC_LEFT 33
#define POW_MC_RIGHT 34
#define POW_LEFT_PUMP 35
#define POW_RIGHT_PUMP 36
#define POW_LEFT_FAN 37
#define POW_RIGHT_FAN 38

#define RELAY_PDU_OUTPUTS 0x8030F03

void relayPduOutputs(void * pvParameters);
extern void (*relayPduPtr)(void*);

typedef enum pdu_out_status_bits{
    aux = 0,
    brake_light,
    battery,
    bmu,
    vcu,
    dcu,
    mc_left,
    mc_right,
    left_pump,
    right_pump,
    left_fan,
    right_fan,
}pdu_out_status_bits;

#endif