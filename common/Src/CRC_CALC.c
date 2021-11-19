#include "bsp.h"


/*
 * If CRC can be accelerated via hardware, enable it in the HAL and use this
 * function. Otherwise, we provide a software solution. Be advised that
 * it is much slower than the hardware alternative, so it should not be used
 * on any performance critical code.
 */
#ifdef HAL_CRC_MODULE_ENABLED

const uint32_t BUFFER_SIZE = (uint32_t)(6);

uint8_t calculate_base_CRC( uint8_t * data_bytes )
{
    uint8_t crc_value = (uint8_t)HAL_CRC_Calculate(&hcrc, (uint32_t*)(data_bytes), BUFFER_SIZE);
    return (uint8_t)(crc_value ^ 0xff);
}

#else /* Hardware CRC support not available */

uint8_t autosar_crc_lookup_table[256];
uint8_t calculate_base_CRC( uint8_t * data_bytes )
{
    uint8_t crc = 0xff;
    uint8_t index = 0;
    for (int i = 0; i < 6; i++)
    {
        index = (uint8_t)( data_bytes[i] ^ crc );
        crc = (uint8_t)( autosar_crc_lookup_table[index] );
    }

    return (uint8_t)( crc ^ 0xff );
}
void generate_CRC_lookup_table()
{
    uint8_t top_bit = 128;
    uint8_t procan_CRC_Poly = 0x2f;
    for (int i = 0; i < 256; i++)
    {
        uint8_t remainder = (uint8_t)i;
        for(int j = 0; j < 8; j++)
        {
            if ((remainder & top_bit) != 0)
                remainder = (uint8_t)((remainder << 1) ^procan_CRC_Poly);
            else
                remainder = (uint8_t)(remainder << 1);
        }

        autosar_crc_lookup_table[i] = remainder;
    }
}

#endif /* defined(HAL_CRC_MODULE_ENABLED) */
