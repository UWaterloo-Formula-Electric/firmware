#ifdef __STM32F7xx_HAL_CRC_H

#ifndef __CRC_CALC_H
#define __CRC_CALC_H

uint8_t calculate_base_CRC( uint8_t * data_bytes );
void generate_CRC_lookup_table();

#endif /* defined(__CRC_CALC_H)*/

#endif /* defined(__STM32F7xx_HAL_CRC_H) */
