#include "errorHandler.h"
#include "main.h"
#include "freertos.h"
#include "stdio.h"
#include "stm32f7xx_hal.h"
#include "debug.h"
#include "BMU_dtc.h"

void _handleError(char *file, int line)
{
#ifndef PRODUCTION_ERROR_HANDLING
  printf("ERROR!: File %s, line %d\n", file, line);
  HAL_GPIO_WritePin(ERROR_LED_PORT, ERROR_LED_PIN, GPIO_PIN_SET);
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
    taskDISABLE_INTERRUPTS();
  }
  sendDTC_FATAL_BMU_ERROR();
  while(1);
#else
  // TODO: create production error handler
#endif
}

int log_assert_violation(char *file, int line, char *condition)
{
    ERROR_PRINT("ASSERT FAILURE: (%s): %s:%d\n", condition, file, line);
    return 1;
}
