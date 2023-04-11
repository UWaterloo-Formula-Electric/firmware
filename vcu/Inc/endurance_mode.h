#ifndef ENDURANCE_MODE_H
#define ENDURANCE_MODE_H
#include "stm32f7xx_hal.h"
#include "stdint.h"
#include "stdbool.h"

// Config
#define NUMBER_OF_LAPS_TO_COMPLETE_DEFAULT (44U)
#define ENDURANCE_MODE_BUFFER (1.05f)

void endurance_mode_EM_callback(void);
void set_laps_to_complete(float laps);
void trigger_lap(void);
void toggle_endurance_mode(void);
HAL_StatusTypeDef set_initial_soc(float initial_soc_value);
HAL_StatusTypeDef set_num_laps(uint32_t num_laps_value);
void set_num_laps_completed(uint32_t num_laps_completed_value);
void set_in_endurance_mode(bool in_endurance_mode_bool);
void set_em_kP(float em_kP_value);
void set_em_kI(float em_kI_value);
float get_initial_soc(void);
uint32_t get_num_laps(void);
uint32_t get_num_laps_complete(void);
bool get_in_endurance_mode(void);
float get_em_kP(void);
float get_em_kI(void);

#endif
