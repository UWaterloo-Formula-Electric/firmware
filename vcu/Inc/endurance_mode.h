#ifndef ENDURANCE_MODE_H
#define ENDURANCE_MODE_H
#include "stm32f7xx_hal.h"
#include "stdint.h"

// Config
#define NUMBER_OF_LAPS_TO_COMPLETE_DEFAULT (990)
#define ENDURANCE_MODE_BUFFER (1.05f)

void endurance_mode_EM_callback(void);
void set_lap_limit(uint32_t laps);
void trigger_lap(void);
void toggle_endurance_mode(void);

#endif
