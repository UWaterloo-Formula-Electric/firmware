#define CAN_TX GPIO_NUM_12// check schematic
#define CAN_RX GPIO_NUM_13
#define CS_STR_GAU_1_PIN 5
#define CS_STR_GAU_2_PIN 6
#define CS_STR_GAU_3_PIN 7
#define CS_DAM_POS_PIN 15
#define DAC_CH1_PIN 17
#define DAC_CH2_PIN 18
#define CS_ROT_DIS_PIN 9
#define CS_BRK_TEMP_PIN 10
#define CS_CLN_TEMP_1_PIN 21
#define CS_CLN_TEMP_2_PIN 33
#define CS_WHL_SPD_PIN 34
#define CS_TREW_PRES_PIN 36
#define CS_TRE_TEMP_PIN 37
#define CS_CLN_FLW_PIN 38
#define ENC_OUT_PIN 39
#define CS_POT_PIN 40
#define SCK_PIN 2
#define SDO_PIN 1

//IO19 IO20


// #define 


void taskRegister (void);
esp_err_t CAN_init (void);