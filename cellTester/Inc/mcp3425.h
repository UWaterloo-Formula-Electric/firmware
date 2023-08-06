#ifndef MCP3425_DRIVER
#define MCP3425_DRIVER

#include "debug.h"
#include "i2c.h"

// MCP3425
#define DEVICE_CODE 0b1101

HAL_StatusTypeDef mcp3425_configure(I2C_HandleTypeDef *i2cmodule);
HAL_StatusTypeDef mcp3425_read(I2C_HandleTypeDef *i2cmodule, int16_t *data_out);

#endif
