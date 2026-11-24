#include "bsp.h"
#include "charger.h"
#include "controlStateMachine.h"
#include "chargerI2c.h"
#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "state_machine.h"
#include "watchdog.h"

//verifying youre talking to the right chip
bqCommStatus_t chargerVerifyChip(uint8_t *data){
    if (data == NULL){
        return CHARGER_INVALID_ARG; //invalid pointer
    }
    HAL_StatusTypeDef status = chargerReadRegister(BQ_PART_INFO_REG, data);

    if (status != HAL_OK){
        return CHARGER_BUS_ERROR; // I2C read failed
    }

    uint8_t partInfoData = chargerDecodePartNumber(*data);

    if (partInfoData != BQ_PART_NUMBER_EXPECTED){
        return CHARGER_WRONG_CHIP; // wrong chip ID read, mark the charger as unavailalbe, release CE
    }
    return CHARGER_OK;   
}  

uint8_t chargerDecodeRevision(uint8_t data){ //pass by val since its small and we don't want to worry abotu null ptrs
    return (data & BQ_DEV_REV_MASK) >> BQ_DEV_REV_SHIFT; // shift and mask to get revision number
}

uint8_t chargerDecodePartNumber(uint8_t data){
    return (data & BQ_PART_NUM_MASK) >> BQ_PART_NUM_SHIFT; // shift and mask to get part number
}

chargerState_t chargerGetState(uint8_t stat1Level, uint8_t stat2Level){
    if (!stat1Level && stat2Level){
        return CHARGE_IN_PROGRESS;
    }
    else if (stat1Level && !stat2Level){
        return CHARGE_DONE;
    }
    else if (!stat1Level && !stat2Level){
        return CHARGE_FAULT;
    }
    else{
        return CHARGE_DISABLED;
    }
}

// read one 16 bit register and compare only the bits in mask
static bqCommStatus_t chargerCheckReg16(uint8_t reg, uint16_t expected, uint16_t mask){
    uint16_t actual = 0;
    if (chargerRead16BitRegister(reg, &actual) != HAL_OK){
        return CHARGER_BUS_ERROR;
    }
    if ((actual & mask) != (expected & mask)){
        ERROR_PRINT("Charger config mismatch reg 0x%02X: expected 0x%04X, read 0x%04X\n",
            reg, expected & mask, actual & mask);
        return CHARGER_VERIFY_MISMATCH;
    }
    return CHARGER_OK;
}

// 8 bit version of the above
static bqCommStatus_t chargerCheckReg8(uint8_t reg, uint8_t expected, uint8_t mask){
    uint8_t actual = 0;
    if (chargerReadRegister(reg, &actual) != HAL_OK){
        return CHARGER_BUS_ERROR;
    }
    if ((actual & mask) != (expected & mask)){
        ERROR_PRINT("Charger config mismatch reg 0x%02X: expected 0x%02X, read 0x%02X\n",
            reg, expected & mask, actual & mask);
        return CHARGER_VERIFY_MISMATCH;
    }
    return CHARGER_OK;
}

bqCommStatus_t chargerVerifyConfig(void){
    bqCommStatus_t status;
    if ((status = chargerCheckReg16(BQ_CHARGE_VOLTAGE_LIMIT_REG, BQ_VFB_MV_TO_REG(CHARGER_VFB_MV), BQ_VFB_REG_MASK)) != CHARGER_OK){
        return status;
    }
    if ((status = chargerCheckReg16(BQ_CHARGE_CURRENT_LIMIT_REG, BQ_CURRENT_MA_TO_REG(CHARGER_ICHG_MA), BQ_ICHG_REG_MASK)) != CHARGER_OK){
        return status;
    }
    if ((status = chargerCheckReg16(BQ_TERM_CURRENT_LIMIT_REG, BQ_CURRENT_MA_TO_REG(CHARGER_ITERM_MA), BQ_ITERM_MASK)) != CHARGER_OK){
        return status;
    }
    if ((status = chargerCheckReg8(BQ_CONTROL_REG, CHARGER_VRECHG_SETTING, BQ_VRECHG_MASK)) != CHARGER_OK){
        return status;
    }
    // ADC on (bit 7) and continuous (bit 6 = 0)
    if ((status = chargerCheckReg8(BQ_ADC_CONTROL_REG, BQ_ADC_EN_MASK, BQ_ADC_EN_MASK | BQ_ADC_RATE_MASK)) != CHARGER_OK){
        return status;
    }
    return CHARGER_OK;
}

bqCommStatus_t chargerApplyConfig(void){
    if (chargerWrite16BitRegister(BQ_CHARGE_VOLTAGE_LIMIT_REG, BQ_VFB_MV_TO_REG(CHARGER_VFB_MV)) != HAL_OK){
        return CHARGER_BUS_ERROR;
    }
    if (chargerWrite16BitRegister(BQ_CHARGE_CURRENT_LIMIT_REG, BQ_CURRENT_MA_TO_REG(CHARGER_ICHG_MA)) != HAL_OK){
        return CHARGER_BUS_ERROR;
    }
    if (chargerWrite16BitRegister(BQ_TERM_CURRENT_LIMIT_REG, BQ_CURRENT_MA_TO_REG(CHARGER_ITERM_MA)) != HAL_OK){
        return CHARGER_BUS_ERROR;
    }
    // read modify write: 0x17 also holds WD_RST, DIS_CE_PIN and EN_CHG, which must not change here
    if (chargerModifyBits(BQ_CONTROL_REG, BQ_VRECHG_MASK, CHARGER_VRECHG_SETTING) != HAL_OK){
        return CHARGER_BUS_ERROR;
    }
    // ADC on in continuous mode: the default one-shot mode stops after a single conversion
    if (chargerModifyBits(BQ_ADC_CONTROL_REG, BQ_ADC_EN_MASK | BQ_ADC_RATE_MASK, BQ_ADC_EN_MASK) != HAL_OK){
        return CHARGER_BUS_ERROR;
    }
    return chargerVerifyConfig();
}

void chargerTask(void const *argument){
    // verify once at startup: if the chip is missing or wrong, never poll it and never enable CE
    uint8_t chargerPartData = 0;
    bool chargerAvailable = (chargerVerifyChip(&chargerPartData) == CHARGER_OK);
    if (!chargerAvailable){
        ERROR_PRINT("Charger verify failed (part info 0x%02X), charging stays disabled\n", chargerPartData);
    }

    if (registerTaskToWatch(CHARGER_TASK_ID, 2*CHARGER_TASK_PERIOD_MS, false, NULL) != HAL_OK){
        ERROR_PRINT("Failed to register charger task with watch dog!\n");
        Error_Handler();
    }
    TickType_t xLastWakeTime = xTaskGetTickCount();

    // STAT debounce: accept a new state only after CHARGER_STAT_DEBOUNCE_COUNT matching samples in a row
    // Starts as DISABLED bc CE is driven high at GPIO init
    chargerState_t stableState = CHARGE_DISABLED;
    chargerState_t candidateState = CHARGE_DISABLED;
    uint8_t candidateCount = 0;

    uint8_t pollCount = 0; 
    uint8_t faultStatData = 0, stat1Data = 0, stat2Data = 0;
    uint16_t vBatData = 0, iBatData = 0, tsData = 0;

    


    while (1){
        // CE enable removed from the loop: it comes back in step 5 with the main control lockout check

        uint8_t chgStat1Level = (READ_CHARGE_STAT_1 == GPIO_PIN_SET);
        uint8_t chgStat2Level = (READ_CHARGE_STAT_2 == GPIO_PIN_SET);
        chargerState_t sample = chargerGetState(chgStat1Level, chgStat2Level);

        if (sample == stableState){
            candidateCount = 0; // nothing changing, drop any half counted candidate
        }
        else if (sample == candidateState){
            candidateCount++;
            if (candidateCount >= CHARGER_STAT_DEBOUNCE_COUNT){
                stableState = sample; // confirmed, the chip really changed state
                candidateCount = 0;
                DEBUG_PRINT("Charger state changed to %d\n", stableState);
            }
        }
        else{
            candidateState = sample; // new candidate, start counting
            candidateCount = 1;
        }

        if (chargerAvailable && pollCount == 0){
            bool pollOk = true;
            // value must be pre-shifted: pass the mask so bit 5 (WD_RST) is set
            if (chargerModifyBits(BQ_CONTROL_REG, BQ_WATCHDOG_RESET_MASK, BQ_WATCHDOG_RESET_MASK) != HAL_OK){
                ERROR_PRINT("Failed to reset BQ watchdog!\n");
                pollOk = false;
            }
            if (chargerReadRegister(BQ_FAULT_STAT_REG, &faultStatData) != HAL_OK){
                ERROR_PRINT("Failed to read fault status register!\n");
                pollOk = false;
            }
            if (chargerReadRegister(BQ_CHARGE_STAT1_REG, &stat1Data) != HAL_OK){
                ERROR_PRINT("Failed to read charge status 1 register!\n");
                pollOk = false;
            }
            if (chargerReadRegister(BQ_CHARGE_STAT2_REG, &stat2Data) != HAL_OK){
                ERROR_PRINT("Failed to read charge status 2 register!\n");
                pollOk = false;
            }
            if (chargerRead16BitRegister(BQ_VBAT_ADC_REG, &vBatData) != HAL_OK){
                ERROR_PRINT("Failed to read battery voltage register!\n");
                pollOk = false;
            }
            if (chargerRead16BitRegister(BQ_IBAT_ADC_REG, &iBatData) != HAL_OK){
                ERROR_PRINT("Failed to read battery current register!\n");
                pollOk = false;
            }
            if (chargerRead16BitRegister(BQ_TS_ADC_REG, &tsData) != HAL_OK){
                ERROR_PRINT("Failed to read temperature register!\n");
                pollOk = false;
            }

            // only print a complete, fresh set: a failed read leaves stale data in its variable
            if (pollOk){
                DEBUG_PRINT("Charger: state %d, fault 0x%02X, stat1 0x%02X, stat2 0x%02X, vbat raw %u, ibat raw %d, ts raw %u\n",
                    stableState, faultStatData, stat1Data, stat2Data, vBatData, (int16_t)iBatData, tsData);
            }
        }

        pollCount++;
        if (pollCount >= CHARGER_POLL_EVERY_N_LOOPS){
            pollCount = 0;
        }

        watchdogTaskCheckIn(CHARGER_TASK_ID); // every loop: software watchdog timeout is 2 x period
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CHARGER_TASK_PERIOD_MS));
    }
}
