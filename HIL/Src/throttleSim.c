// Copied from vcu/Inc/brakeAndThrottle.h
#define THROTTLE_SIM_A_RELEASED 2096.0f   // THROTT_A_HIGH, pot A is inverted
#define THROTTLE_SIM_A_PRESSED   313.0f   // THROTT_A_LOW
#define THROTTLE_SIM_B_RELEASED 2044.0f   // THROTT_B_LOW
#define THROTTLE_SIM_B_PRESSED  3804.0f   // THROTT_B_HIGH
#define THROTTLE_SIM_DAC_MAX    4095.0f

#include "throttleSim.h"   // brings in stdint and the HAL types
#include "i2cDac.h"        // i2cDacSetCodes


static uint16_t percentToCode(float released, float pressed, float percent)
{
    float count = released + (pressed - released) * percent / 100.0f;
    if (count < 0.0f) {
        count = 0.0f;
    }
    if (count > THROTTLE_SIM_DAC_MAX) {
        count = THROTTLE_SIM_DAC_MAX;
    }

    return (uint16_t)(count + 0.5f);


}

HAL_StatusTypeDef throttleSimSetPercents(float percentA, float percentB,
                                         uint16_t *codeA, uint16_t *codeB)
{
    if (codeA == NULL || codeB == NULL) {
        return HAL_ERROR;
    }

    *codeA = percentToCode(THROTTLE_SIM_A_RELEASED, THROTTLE_SIM_A_PRESSED, percentA);
    *codeB = percentToCode(THROTTLE_SIM_B_RELEASED, THROTTLE_SIM_B_PRESSED, percentB);

    return i2cDacSetCodes(*codeA, *codeB);
}