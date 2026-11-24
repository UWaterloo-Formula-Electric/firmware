#ifndef CHARGER_H
#define CHARGER_H

#include "chargerRegs.h"
#include "main.h"
#include <stdint.h>

// CE is active low: RESET = charging enabled, SET = charging disabled
#define CHARGE_ENABLE_ON           HAL_GPIO_WritePin(CHG_EN_GPIO_Port, CHG_EN_Pin, GPIO_PIN_RESET)
#define CHARGE_ENABLE_OFF          HAL_GPIO_WritePin(CHG_EN_GPIO_Port, CHG_EN_Pin, GPIO_PIN_SET)
#define CHARGER_TASK_PERIOD_MS     100 // 100 ms
#define CHARGER_STAT_DEBOUNCE_COUNT 3  // samples in a row to accept a new state: 3 x 100 ms = 300 ms
#define CHARGER_POLL_EVERY_N_LOOPS (5000 / CHARGER_TASK_PERIOD_MS) // full I2C poll every 5 s

#define CHARGER_VFB_MV              1536            // datasheet default FB ref
#define CHARGER_ICHG_MA             2000            // max charge current
#define CHARGER_ITERM_MA            250             // TODO: datasheet minimum, must be below ICHG
#define CHARGER_VRECHG_SETTING      (0x3 << 6)      // TODO: 97.6% (Li-ion recommendation); LiFePO4 uses 0x0

// register encodings (datasheet 6.5.1, 6.5.2, 6.5.8)
#define BQ_VFB_MV_TO_REG(mv)        ((uint16_t)(((mv) - 1504) / 2))
#define BQ_CURRENT_MA_TO_REG(ma)    ((uint16_t)(((ma) / 50) << 2))   // ICHG_REG and ITERM: bits [..:2]

typedef enum {
    CHARGER_OK, // charger IC is present and responding
    CHARGER_BUS_ERROR, // I2C read failed (single attempt, no retry)
    CHARGER_WRONG_CHIP, // wrong chip ID read, mark the charger as unavailalbe, release CE
    CHARGER_INVALID_ARG, // invalid pointer 
    CHARGER_VERIFY_MISMATCH // config readback differs from what was written 
} bqCommStatus_t;

bqCommStatus_t chargerVerifyChip(uint8_t *data);

// Writes the charger configuration, then verifies it (chargerVerifyConfig). Call at startup and
// again after the BQ watchdog expires (it resets ICHG_REG and ADC_EN). Worst case ~20 ms on a dead bus.
bqCommStatus_t chargerApplyConfig(void);

// Reads the config registers back and compares the fields that matter
bqCommStatus_t chargerVerifyConfig(void);

// two helper functions are used to decode the partand revision nubmer from the part info register
uint8_t chargerDecodeRevision(uint8_t data);

uint8_t chargerDecodePartNumber(uint8_t data);

typedef enum {
    CHARGE_IN_PROGRESS, //stat1 low, stat2 high
    CHARGE_DONE, //stat1 high, stat2 low
    CHARGE_FAULT, // stat1 low, stat2 low
    CHARGE_DISABLED //stat1 high, stat2 high
} chargerState_t;

chargerState_t chargerGetState(uint8_t stat1Level, uint8_t stat2Level);


#endif  // CHARGER_H