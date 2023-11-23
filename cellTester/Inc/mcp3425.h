#ifndef MCP3425_DRIVER_H
#define MCP3425_DRIVER_H

#include "debug.h"
#include "i2c.h"

// MCP3425
#define MCP3425_DEVICE_CODE 0b1101
#define MCP3425_ADDR_CODE 0b000
#define MCP3425_ADDR_BYTE ((MCP3425_DEVICE_CODE << 3) | 0b000)
#define I2C_INIT_TRIALS 5
#define I2C_INIT_TIMEOUT 1000

#define MCP3425_CONFIG_BYTE_DEFAULT_CONFIG 0b10011000
#define MCP3425_CONFIG_BYTE_SET_16BIT_RESOLUTION 0b1000
#define MCP3425_CONFIG_BYTE_SET_PGA_GAIN_1VV 0b00
#define MCP3425_CONFIG_BYTE (MCP3425_CONFIG_BYTE_DEFAULT_CONFIG | MCP3425_CONFIG_BYTE_SET_16BIT_RESOLUTION)
#define MCP3425_CONFIG_BYTE_SIZE 1U
#define MCP3425_CONFIG_RDY_BIT_MASK 0b10000000

// When R/W bit is 1, the device outputs the conversion data, otherwise it expects a config byte
#define MCP3425_READ_BIT 1
#define MCP3425_PGA_GAIN 1 // Set in the config byte

#define MCP3425_RX_BUFFER_SIZE 3

HAL_StatusTypeDef mcp3425_adc_configure(I2C_HandleTypeDef *i2c_hdr);
HAL_StatusTypeDef mcp3425_adc_read(I2C_HandleTypeDef *i2c_hdr);
extern int16_t adc_1_output_val;
extern int16_t adc_2_output_val;
#endif // MCP3425_DRIVER_H
