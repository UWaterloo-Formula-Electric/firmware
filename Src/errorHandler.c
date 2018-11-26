#include <errorHandler.h>
#include "main.h"
#include "freertos.h"
#include "stdio.h"
#include "stm32f7xx_hal.h"
#include "VCU_F7_dtc.h"

void _handleError(char *file, int line)
{
#ifndef PRODUCTION_ERROR_HANDLING
  printf("ERROR!: File %s, line %d\n", file, line);
  HAL_GPIO_WritePin(ERROR_LED_PORT, ERROR_LED_PIN, GPIO_PIN_SET);
  taskDISABLE_INTERRUPTS();
  sendDTC_FATAL_VCU_F7_ERROR();
#else
  // TODO: create production error handler
#endif
}
