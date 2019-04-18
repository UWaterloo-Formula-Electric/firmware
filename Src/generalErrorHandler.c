#include "main.h"
#include "freertos.h"
#include "stdio.h"
#include "userCan.h"
#include "bsp.h"
#include "string.h"
#include "generalErrorHandler.h"
#include AUTOGEN_HEADER_NAME(BOARD_NAME)
#include AUTOGEN_DTC_HEADER_NAME(BOARD_NAME)

#define SEND_FATAL_DTC CAT(sendDTC_FATAL_, CAT(BOARD_NAME, _ERROR))

// Reset the debug uart
// This is done to clear the UART in case it is being used by the debug task,
// that way we can send an error message
HAL_StatusTypeDef resetUART()
{
    if (HAL_UART_DeInit(&DEBUG_UART_HANDLE) != HAL_OK) {
        return HAL_ERROR;
    }

    if (HAL_UART_Init(&DEBUG_UART_HANDLE) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

void _handleError(char *file, int line)
{
#ifndef PRODUCTION_ERROR_HANDLING
  const char errorStringFile[] = "Error!: File ";
  const char errorStringLine[] = " line ";
  char lineNumberString[10];

  taskDISABLE_INTERRUPTS();

  HAL_GPIO_WritePin(ERROR_LED_PORT, ERROR_LED_PIN, GPIO_PIN_SET);

  SEND_FATAL_DTC();

  if (resetUART() != HAL_OK) {
      // Can't really do anything else
      while (1);
  }

  HAL_UART_Transmit(&DEBUG_UART_HANDLE, ((uint8_t *)errorStringFile), strlen(errorStringFile), 1000);
  HAL_UART_Transmit(&DEBUG_UART_HANDLE, ((uint8_t *)file), strlen(file), 1000);
  HAL_UART_Transmit(&DEBUG_UART_HANDLE, ((uint8_t *)errorStringLine), strlen(errorStringLine), 1000);
  snprintf(lineNumberString, sizeof(lineNumberString), "%d\n", line);
  HAL_UART_Transmit(&DEBUG_UART_HANDLE, ((uint8_t *)lineNumberString), strlen(lineNumberString), 1000);

  while(1);
#else
  // TODO: create production error handler
#endif
}
