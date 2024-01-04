#define CAN_TX GPIO_NUM_12// check schematic
#define CAN_RX GPIO_NUM_13

// #define 


void taskRegister (void);
esp_err_t CAN_init (void);