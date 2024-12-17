#ifndef USER_INIT_H
#define USER_INIT_H
#include "driver/dac_oneshot.h"

//GPIO Pins
#define BR_IR_TEMP 17
#define BR_PRESS 18
#define WHE_ENC 9
#define BC_ROT_ENC 10
#define HL_EFF 33
#define MC_FLOW 34

//DAC Channels
#define DAC_CHAN_BRAKE_IR DAC_CHAN_0

//CAN Pins
#define CAN_TX GPIO_NUM_12
#define CAN_RX GPIO_NUM_13

typedef enum PwmChannel_E {
    PwmChannel_HallEff = 0,
} PwmChannel_E;

typedef enum PwmTimer_E {
    PwmTimer_HallEff = 0,
} PwmTimer_E;

extern dac_oneshot_handle_t brake_ir_handle;

void taskRegister (void);
esp_err_t CAN_init (void);
esp_err_t dac_init (void);
esp_err_t pwm_init (void);

#endif/*USER_INIT_H*/