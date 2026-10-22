#include "charger.h"
#include "controlStateMachine.h"
#include "bsp.h"
#include "i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "debug.h"
#include "String.h"
#include "state_machine.h"


HAL_StatusTypeDef chargerReadRegister(uint8_t reg, uint8_t *data);

uint8_t data = 0;
HAL_StatusTypeDef status;

// function to read a the part info register from charger IC 
