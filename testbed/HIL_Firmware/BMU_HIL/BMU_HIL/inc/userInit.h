#ifndef USER_INIT_H
#define USER_INIT_H

/***********************************
********** DEFINITIONS *************
************************************/

//pins
#define CAN_HIL_TX        GPIO_NUM_36
#define CAN_HIL_RX        GPIO_NUM_35
#define CAN_CHRGR_TX      GPIO_NUM_39
#define CAN_CHRGR_RX      GPIO_NUM_38

#define SPI_MOSI_PIN      GPIO_NUM_4
#define SPI_CLK_PIN       GPIO_NUM_5

#define ISOSPI_MOSI_PIN   GPIO_NUM_36
#define ISOSPI_MISO_PIN   GPIO_NUM_34
#define ISOSPI_CLK_PIN    GPIO_NUM_35

#define BATT_POS_CS       GPIO_NUM_11
#define BATT_NEG_CS       GPIO_NUM_10
#define HV_NEG_OUT_CS     GPIO_NUM_6
#define HV_POS_OUT_CS     GPIO_NUM_7
#define HV_SHUNT_POS_CS   GPIO_NUM_9
#define HV_SHUNT_NEG_CS   GPIO_NUM_8
#define AMS_CS            GPIO_NUM_37

//ids
#define CAN_HIL_ID     0
#define CAN_CHRGR_ID   1

//utils
#define NOT_USED       -1
#define HZ_PER_MHZ     (1000 * 1000)

/***********************************
***** FUNCTION DECLERATIONS ********
************************************/
esp_err_t CAN_init (void);
esp_err_t SPI_init(void);
esp_err_t GPIO_init (void);

#endif/*USER_INIT_H*/