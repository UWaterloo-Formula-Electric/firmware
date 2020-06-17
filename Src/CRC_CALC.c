#include "bsp.h"

CRC_HandleTypeDef crc_handle;
uint32_t buffer_size = (uint32_t)(6);

uint8_t calculate_base_CRC( uint8_t * data_bytes )
{
    uint8_t crc_value = (uint8_t)HAL_CRC_Calculate(&crc_handle, (uint32_t*)(data_bytes), buffer_size);
    return (crc_value ^ 0xff);
}

void config_crc_handle()
{
    crc_handle.Instance = CRC;
    crc_handle.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_DISABLE;
    crc_handle.Init.GeneratingPolynomial = 0x2f;
    crc_handle.Init.CRCLength = CRC_POLYLENGTH_8B;
    crc_handle.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
    
    if (HAL_CRC_Init(&crc_handle) != HAL_OK)
    {
        Error_Handler();
    }
}