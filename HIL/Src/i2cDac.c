#include "i2cDac.h"
 
#include "FreeRTOS.h"
#include "task.h"      // vTaskDelay, pdMS_TO_TICKS
#include "bsp.h"       // LDAC_n_HIGH / LDAC_n_LOW macros
#include "debug.h"     // ERROR_PRINT
#include "i2cBus.h"    // i2cWrite, i2cIsDeviceReady
 
#define I2C_DAC_BUS          I2C_BUS_1
#define I2C_DAC_ADDRESS7     0x60U   // 1100 A2 A1 A0, with A2..A0 = 000
#define I2C_DAC_FRAME_LENGTH 6U      // 2 channels x 3 bytes
#define I2C_DAC_LDAC_PULSE_MS 1U     // 210 ns is datasheet min
 
// Command byte fields (MCP4728 datasheet, Multi-Write command)
#define MCP4728_CMD_MULTI_WRITE 0x40U   // C2 C1 C0 W1 W0 = 010 00, in bits 7..3
#define MCP4728_CHANNEL_A       0U      // DAC1 DAC0 = 00
#define MCP4728_CHANNEL_B       1U      // DAC1 DAC0 = 01
#define MCP4728_UDAC_WAIT       1U      // 1 = wait for LDAC
 
// Settings in the top nibble of the second byte: VREF PD1 PD0 GX
#define MCP4728_SETTINGS_VDD_REF_GAIN1 0x0U   // VREF=0, PD=00, GX=0

HAL_StatusTypeDef i2cDacInit(void)
{
    LDAC_1_HIGH;
    LDAC_2_HIGH;
    LDAC_3_HIGH;
    LDAC_4_HIGH;
    return HAL_OK;
}

// make sure we use the input registers instead of immediately pushing codes

HAL_StatusTypeDef i2cDacProbe(void)
{
    return i2cIsDeviceReady(I2C_DAC_BUS, I2C_DAC_ADDRESS7 << 1U);

    // shift address left by 1 because HAL expects 8-bit address (7-bit + R/W bit)
}

static void packChannel(uint8_t channel, uint16_t code, uint8_t *out)
{
    out[0] = (uint8_t)( MCP4728_CMD_MULTI_WRITE | (channel << 1) | MCP4728_UDAC_WAIT);   // command byte
    out[1] = (uint8_t)((MCP4728_SETTINGS_VDD_REF_GAIN1 << 4) | (code >> 8));  // settings + top 4 bits of code
    out[2] = (uint8_t)(code & 0xFF);   // low 8 bits of code
}

// each channel takes 3 bytes, we need 6 bytes

static HAL_StatusTypeDef buildFrame(uint16_t codeA, uint16_t codeB, uint8_t *frame)
{
    if (codeA > I2C_DAC_CODE_MAX || codeB > I2C_DAC_CODE_MAX || frame == NULL) // lets say code = 4099, which is bigger than our max code 4095
    {
        ERROR_PRINT("i2cDacSetCodes: invalid code(s) %u, %u\n", codeA, codeB);
        return HAL_ERROR;
    }

    packChannel(MCP4728_CHANNEL_A, codeA, &frame[0]);
    packChannel(MCP4728_CHANNEL_B, codeB, &frame[3]);

    return HAL_OK;
}

static void pulseLdac(void)
{
    LDAC_1_LOW;                                       // falling edge: chip 1 loads
    vTaskDelay(pdMS_TO_TICKS(I2C_DAC_LDAC_PULSE_MS)); // hold low (needs only 210 ns)
    LDAC_1_HIGH;                                      // park high again
}

HAL_StatusTypeDef i2cDacSetCodes(uint16_t codeA, uint16_t codeB)
{
    uint8_t frame[I2C_DAC_FRAME_LENGTH];
 
    if (buildFrame(codeA, codeB, frame) != HAL_OK) { // build
        ERROR_PRINT("DAC code out of range: A=%u B=%u\n", codeA, codeB);
        return HAL_ERROR;
    }
 
    if (i2cWrite(I2C_DAC_BUS, I2C_DAC_ADDRESS7 << 1U, frame, I2C_DAC_FRAME_LENGTH) != HAL_OK) { // send
        return HAL_ERROR;   // i2cWrite already printed why, kills the process before pulsing LDAC to send
    }
 
    pulseLdac();            // both channels change together, now (write)
    return HAL_OK;
}