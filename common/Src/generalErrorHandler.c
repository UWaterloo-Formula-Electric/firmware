#include "main.h"
#include "FreeRTOS.h"
#include "stdio.h"
#include "userCan.h"
#include "bsp.h"
#include "string.h"
#include "generalErrorHandler.h"
#include "watchdog.h"
#include "debug.h"

#ifndef DISABLE_CAN_FEATURES
#include AUTOGEN_HEADER_NAME(BOARD_NAME)
#include AUTOGEN_DTC_HEADER_NAME(BOARD_NAME)


#if !IS_BOARD_NUCLEO_F7 
#define PRODUCTION_ERROR_HANDLING
#endif

#define SEND_FATAL_DTC CAT(sendDTC_FATAL_, CAT(BOARD_NAME_UPPER, _ERROR))
#define SEND_CRITICAL_DTC CAT(sendDTC_CRITICAL_, CAT(BOARD_NAME_UPPER, _ERROR))
#endif


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

#ifdef DISABLE_CAN_FEATURES
void _handleError(char *file, int line)
{
  const char errorStringFile[] = "Error!: File ";
  const char errorStringLine[] = " line ";
  char lineNumberString[10];

  taskDISABLE_INTERRUPTS();

  if (resetUART() != HAL_OK) {
      // Can't really do anything else
    while(1)
    {
      watchdogRefresh();
    }
  }

  HAL_UART_Transmit(&DEBUG_UART_HANDLE, ((uint8_t *)errorStringFile), strlen(errorStringFile), 1000);
  HAL_UART_Transmit(&DEBUG_UART_HANDLE, ((uint8_t *)file), strlen(file), 1000);
  HAL_UART_Transmit(&DEBUG_UART_HANDLE, ((uint8_t *)errorStringLine), strlen(errorStringLine), 1000);
  snprintf(lineNumberString, sizeof(lineNumberString), "%d\n", line);
  HAL_UART_Transmit(&DEBUG_UART_HANDLE, ((uint8_t *)lineNumberString), strlen(lineNumberString), 1000);

  while(1)
  {
    DEBUG_PRINT("Unrecoverable Error Set\r\n");
    watchdogRefresh();
  }
}
#else

// Flag to ensure we only trigger error handling once
bool errorOccured = false;
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
    while(1)
    {
      watchdogRefresh();
    }
  }

  HAL_UART_Transmit(&DEBUG_UART_HANDLE, ((uint8_t *)errorStringFile), strlen(errorStringFile), 1000);
  HAL_UART_Transmit(&DEBUG_UART_HANDLE, ((uint8_t *)file), strlen(file), 1000);
  HAL_UART_Transmit(&DEBUG_UART_HANDLE, ((uint8_t *)errorStringLine), strlen(errorStringLine), 1000);
  snprintf(lineNumberString, sizeof(lineNumberString), "%d\n", line);
  HAL_UART_Transmit(&DEBUG_UART_HANDLE, ((uint8_t *)lineNumberString), strlen(lineNumberString), 1000);

  while(1)
  {
    watchdogRefresh();
  }
#else
  if (!errorOccured) {
    HAL_GPIO_WritePin(ERROR_LED_PORT, ERROR_LED_PIN, GPIO_PIN_SET);
	
#if !BOARD_IS_WSB(BOARD_ID)
	sendDTC_WARNING_ERROR_HANDLER(line);
    SEND_FATAL_DTC();
#else
    SEND_CRITICAL_DTC();
#endif
    // Trigger whatever error handling this board has
    DTC_Fatal_Callback(BOARD_ID);
    errorOccured = true;
  }
#endif
}
#endif // DISABLE_CAN_FEATURES