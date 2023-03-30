#ifndef STM32F7XX_HAL_H
#define STM32F7XX_HAL_H
#include "fake_hal_defs.h"

/* IO definitions (access restrictions to peripheral registers) */
#define     __O     volatile
#define     __IO    volatile

/* following defines should be used for structure members */
#define     __IM     volatile const
#define     __OM     volatile
#define     __IOM    volatile

typedef struct
{
    __IO uint32_t  DR;
    __IO uint8_t   IDR;
    uint8_t        RESERVED0;
    uint16_t       RESERVED1;
    __IO uint32_t  CR;
    uint32_t       RESERVED2;
    __IO uint32_t  INIT;
    __IO uint32_t  POL;
} CRC_TypeDef;

typedef struct
{
    uint8_t DefaultPolynomialUse;   
    uint8_t DefaultInitValueUse;  
    uint32_t GeneratingPolynomial;
    uint32_t CRCLength;
    uint32_t InitValue;
    uint32_t InputDataInversionMode;
    uint32_t OutputDataInversionMode;
} CRC_InitTypeDef;

typedef enum 
{
	HAL_UNLOCKED = 0x00U,
	HAL_LOCKED   = 0x01U  
} HAL_LockTypeDef;

typedef enum
{
	HAL_CRC_STATE_RESET     = 0x00U, 
	HAL_CRC_STATE_READY     = 0x01U,
	HAL_CRC_STATE_BUSY      = 0x02U,
	HAL_CRC_STATE_TIMEOUT   = 0x03U,
	HAL_CRC_STATE_ERROR     = 0x04U
} HAL_CRC_StateTypeDef;

typedef struct
{
    CRC_TypeDef                 *Instance;
    CRC_InitTypeDef             Init;
    HAL_LockTypeDef             Lock;
    __IO HAL_CRC_StateTypeDef   State;
    uint32_t InputDataFormat;
} CRC_HandleTypeDef;

#endif
