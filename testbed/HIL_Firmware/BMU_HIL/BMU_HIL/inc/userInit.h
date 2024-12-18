#ifndef USER_INIT_H
#define USER_INIT_H

/***********************************
********** DEFINITIONS *************
************************************/
/////////////
//SPI Pins
/////////////
#define CAN_HIL_TX            GPIO_NUM_36
#define CAN_HIL_RX            GPIO_NUM_35
#define CAN_CHRGR_TX          GPIO_NUM_39
#define CAN_CHRGR_RX          GPIO_NUM_38

#define SPI_MOSI_PIN          GPIO_NUM_4
#define SPI_CLK_PIN           GPIO_NUM_5

#define ISOSPI_MOSI_PIN       GPIO_NUM_36
#define ISOSPI_MISO_PIN       GPIO_NUM_34
#define ISOSPI_CLK_PIN        GPIO_NUM_35

#define BATT_POS_CS           GPIO_NUM_11
#define BATT_NEG_CS           GPIO_NUM_10
#define HV_NEG_OUT_CS         GPIO_NUM_6
#define HV_POS_OUT_CS         GPIO_NUM_7
#define HV_SHUNT_POS_CS       GPIO_NUM_9
#define HV_SHUNT_NEG_CS       GPIO_NUM_8
#define AMS_CS                GPIO_NUM_37

//ids
#define CAN_HIL_ID     0
#define CAN_CHRGR_ID   1

//utils
#define NOT_USED       -1
#define HZ_PER_MHZ     (1000 * 1000)

/////////
//GPIO
/////////
#define BPSD_RESET_PRESS_PIN  GPIO_NUM_21
#define AMS_RESET_PRESS_PIN   GPIO_NUM_33
#define IMD_RESET_PRESS_PIN   GPIO_NUM_3

#define IMD_STATUS_PIN        GPIO_NUM_9
#define IMD_STATUS_TIMER      1
#define IMD_STATUS_FREQ       20E6
#define IMD_FAULT_PIN         GPIO_NUM_10

#define DCDC_ON_PIN           GPIO_NUM_34

#define FAN_TACH_PIN          GPIO_NUM_2 //Good for pwm
#define FAN_TACH_TIMER        0
#define FAN_PWM_PIN           GPIO_NUM_1

#define HVD_PIN               GPIO_NUM_11
#define CBRB_PRESS_PIN        GPIO_NUM_14
#define TSMS_FAULT_PIN        GPIO_NUM_13
#define IL_CLOSE_PIN          GPIO_NUM_12

#define CONT_NEG_PIN          GPIO_NUM_37
#define CONT_POS_PIN          GPIO_NUM_40

#define RMT_RX_CHANNEL RMT_CHANNEL_0  // RMT channel for measuring duty cycle on FAN PWM


/***********************************
***** FUNCTION DECLARATIONS ********
************************************/
esp_err_t CAN_init (void);
esp_err_t SPI_init(void);
esp_err_t GPIO_init (void);

#endif/*USER_INIT_H*/