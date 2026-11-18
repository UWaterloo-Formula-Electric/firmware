#ifndef THROTTLE_SIM_H
#define THROTTLE_SIM_H

#include "stm32f7xx_hal.h"
#include <stdint.h>

// Drive pot A and pot B to the given pedal percentages (0 = released, 100 = fully pressed).
// Values outside 0..100 are allowed, to provoke out-of-range faults.
// The DAC codes actually sent are returned through codeA and codeB.
HAL_StatusTypeDef throttleSimSetPercents(float percentA, float percentB,
                                         uint16_t *codeA, uint16_t *codeB);

#endif