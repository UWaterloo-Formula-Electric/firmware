#ifndef USER_INIT_H
#define USER_INIT_H
#include "driver/spi_master.h"

/* Initializing all required peripherals (CAN, I2C, SPI) */
// Vehicle CAN
#define CAN_RX_VEH GPIO_NUM_35
#define CAN_TX_VEH GPIO_NUM_36
// Charger CAN
#define CAN_RX_CHRGR GPIO_NUM_38
#define CAN_TX_CHRGR GPIO_NUM_39

// BMU isoSPI
#define SPI_MISO GPIO_NUM_41
#define SPI_SCLK GPIO_NUM_42
#define SPI_MOSI GPIO_NUM_41 //??
#define SPI_CHIP_SELECT GPIO_NUM_41 //??

// BMU I2C
#define BMU_SDI GPIO_NUM_4
#define BMI_SCK GPIO_NUM_5

void taskRegister(void);
esp_err_t CAN_init(void);
void bmu_input_init(void);

#endif /* USER_INIT_H */