#include "bsp.h"

const uint32_t BUFFER_SIZE = (uint32_t)(6);

uint8_t calculate_base_CRC( uint8_t * data_bytes )
{
    uint8_t crc_value = (uint8_t)HAL_CRC_Calculate(&hcrc, (uint32_t*)(data_bytes), BUFFER_SIZE);
    return (uint8_t)(crc_value ^ 0xff);
}
