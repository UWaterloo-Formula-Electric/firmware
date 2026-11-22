/**
  *****************************************************************************
  * @file    spiBus.h
  * @brief   Generic transfer wrappers for the bench SPI buses
  *****************************************************************************
  */

#ifndef SPI_BUS_H
#define SPI_BUS_H

#include "stm32f7xx_hal.h"

typedef enum {
    SPI_BUS_4 = 0,
    SPI_BUS_5,
    NUM_SPI_BUSES
} SpiBus_t;

HAL_StatusTypeDef spiTransfer(SpiBus_t bus, uint8_t *txData, uint8_t *rxData, uint16_t length);
HAL_StatusTypeDef spiWrite(SpiBus_t bus, uint8_t *txData, uint16_t length);
HAL_StatusTypeDef spiRead(SpiBus_t bus, uint8_t *rxData, uint16_t length);

#endif /* SPI_BUS_H */
