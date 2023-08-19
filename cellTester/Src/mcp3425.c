#include "mcp3425.h"
#include "debug.h"
#include "task.h"
#include "i2c.h"
#include "temperature.h"

uint8_t rbuffer_1[MCP3425_RX_BUFFER_SIZE] = {0U, 0U, 0U};
uint8_t rbuffer_2[MCP3425_RX_BUFFER_SIZE] = {0U, 0U, 0U};
const uint8_t address_byte = ((MCP3425_ADDR_BYTE) << 1); 

int16_t adc_1_output_val = 0;
int16_t adc_2_output_val = 0;

// Match I2C handler to the correct read buffer
static uint8_t* select_rbuffer(I2C_HandleTypeDef *i2c_hdr) {
    uint8_t* rbuffer = NULL;
    if (i2c_hdr == cell_i2c_hdr) {
        rbuffer = rbuffer_1;
    } 
    else if (i2c_hdr == fuse_i2c_hdr) {
        rbuffer = rbuffer_2;
    } 
    return rbuffer;
}

HAL_StatusTypeDef mcp3425_adc_configure(I2C_HandleTypeDef *i2c_hdr) {
    uint8_t tx_payload[4] = {MCP3425_CONFIG_BYTE, 0x00, 0x00, 0x00};

    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit_DMA(i2c_hdr, address_byte, tx_payload, sizeof(tx_payload));

    if (status != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

// Also write the adc reading to provided address
HAL_StatusTypeDef mcp3425_adc_read(I2C_HandleTypeDef *i2c_hdr) {
    uint8_t* rbuffer = select_rbuffer(i2c_hdr);

    HAL_StatusTypeDef status = HAL_I2C_Master_Receive_DMA(i2c_hdr, address_byte, rbuffer, MCP3425_RX_BUFFER_SIZE);

    if (status != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    uint8_t* rbuffer = select_rbuffer(hi2c);
    int16_t adc_raw = (rbuffer[0] << 8) | rbuffer[1];
    // Check RDY bit
    // 0 = Output register has been updated with the latest conversion data.
    uint8_t config_byte = rbuffer[2];
    if (config_byte & MCP3425_CONFIG_RDY_BIT_MASK) {
        // Output register has not been updated
        DEBUG_PRINT_ISR("No MCP3425 ready bit\n");
    }

    if (hi2c->Instance == hi2c1.Instance)
    {        
        adc_1_output_val = adc_raw * MCP3425_PGA_GAIN;
        hdma_i2c1_rx.State = HAL_DMA_STATE_READY;
    }
    else if (hi2c->Instance == hi2c2.Instance)
    {
        adc_2_output_val = adc_raw * MCP3425_PGA_GAIN;
        hdma_i2c2_rx.State = HAL_DMA_STATE_READY;
    }
}
