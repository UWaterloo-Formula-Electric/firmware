/**
  *****************************************************************************
  * @file    errorHandler.c
  * @author  Richard Matthews
  * @brief   Contains assert failure logging function
  *****************************************************************************
  */

#include "errorHandler.h"
#include "main.h"
#include "FreeRTOS.h"
#include "stdio.h"
#include "stm32f7xx_hal.h"
#include "debug.h"
#include "bmu_dtc.h"

int log_assert_violation(char *file, int line, char *condition)
{
    ERROR_PRINT("ASSERT FAILURE: (%s): %s:%d\n", condition, file, line);
    return 1;
}
