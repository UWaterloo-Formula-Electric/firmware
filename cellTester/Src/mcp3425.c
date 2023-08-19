#include "mcp3425.h"
#include "debug.h"
#include "task.h"
#include "i2c.h"
#include "temperature.h"

uint8_t rbuffer_1[MCP3425_RX_BUFFER_SIZE] = {0U, 0U, 0U};
uint8_t rbuffer_2[MCP3425_RX_BUFFER_SIZE] = {0U, 0U, 0U};
const uint8_t address_byte = ((MCP3425_ADDR_BYTE) << 1); 


HAL_StatusTypeDef mcp3425_adc_configure(I2C_HandleTypeDef *i2c_hdr) {
    uint8_t tx_payload[4] = {MCP3425_CONFIG_BYTE, 0x00, 0x00, 0x00};

    // HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(i2c_hdr, address_byte, &config_byte, MCP3425_CONFIG_BYTE_SIZE, HAL_MAX_DELAY);
    HAL_I2C_Master_Transmit_DMA(i2c_hdr, address_byte, tx_payload, MCP3425_CONFIG_BYTE_SIZE+3);

    return HAL_OK;
}

int16_t adc_1_output_val = 0;
int16_t adc_2_output_val = 0;

// Also write the adc reading to provided address
HAL_StatusTypeDef mcp3425_adc_read(I2C_HandleTypeDef *i2c_hdr) {
    uint8_t* rbuffer = NULL;
    if (i2c_hdr == cell_i2c_hdr)
    {
        rbuffer = rbuffer_1;
    }
    else if (i2c_hdr == fuse_i2c_hdr)
    {
        rbuffer = rbuffer_2;
    }
    else 
    {
        DEBUG_PRINT("i2c handler not recognized\r\n");
        return HAL_ERROR;
    }
    // HAL_StatusTypeDef status = HAL_I2C_Master_Receive(i2c_hdr, address_byte, rbuffer, sizeof(rbuffer), HAL_MAX_DELAY);
    HAL_I2C_Master_Receive_DMA(i2c_hdr, address_byte, rbuffer, MCP3425_RX_BUFFER_SIZE);
    return HAL_OK;
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == hi2c1.Instance)
    {        
        int16_t adc_raw = (rbuffer_1[0] << 8) | rbuffer_1[1];
        // Check RDY bit
        // 0 = Output register has been updated with the latest conversion data.
        uint8_t config_byte = rbuffer_1[2];
        if (config_byte & MCP3425_CONFIG_RDY_BIT_MASK) {
            // Output register has not been updated
            DEBUG_PRINT_ISR("No MCP3425 ready bit\n");
        }
        adc_1_output_val = adc_raw;
        hdma_i2c1_rx.State = HAL_DMA_STATE_READY;
    }
    else if (hi2c->Instance == hi2c2.Instance)
    {
        int16_t adc_raw = (rbuffer_2[0] << 8) | rbuffer_2[1];
        // Check RDY bit
        // 0 = Output register has been updated with the latest conversion data.
        uint8_t config_byte = rbuffer_2[2];
        if (config_byte & MCP3425_CONFIG_RDY_BIT_MASK) {
            // Output register has not been updated
            DEBUG_PRINT_ISR("No MCP3425 ready bit\n");
        }
        adc_2_output_val = adc_raw;
        hdma_i2c2_rx.State = HAL_DMA_STATE_READY;
    }
}
