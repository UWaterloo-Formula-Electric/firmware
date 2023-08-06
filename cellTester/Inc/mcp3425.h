#ifndef MCP3425_DRIVER
#define MCP3425_DRIVER

#include "debug.h"
#include "i2c.h"

// MCP3425
#define DEVICE_CODE 0b1101
#define ADDR_CODE (DEVICE_CODE << 3)
#define I2C_INIT_TRIALS 5
#define I2C_INIT_TIMEOUT 1000

HAL_StatusTypeDef mcp3425_device_ready(I2C_HandleTypeDef *i2c_hdr);
HAL_StatusTypeDef mcp3425_configure(I2C_HandleTypeDef *i2c_hdr);
HAL_StatusTypeDef mcp3425_read(I2C_HandleTypeDef *i2c_hdr, int16_t *data_out);

#endif
