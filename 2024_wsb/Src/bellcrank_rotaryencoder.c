#include <stdint.h>
#include <string.h>
#include <stdio.h>  // Include stdio.h for sprintf
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"

ADC_HandleTypeDef hadc1;
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

void StartRotaryEncoderTask(void const * argument);

#define COMMAND_SIZE 1
#define CMD_NO_OPERATION 0x00
#define CMD_READ_POS 0x10
#define CMD_SET_ZERO_POINT 0x70
#define CMD_TIMEOUT pdMS_TO_TICKS(100)

//redirect printf to uart
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
  HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}

void StartRotaryEncoderTask(void const * argument)
{
  /* USER CODE BEGIN 5 */
  uint8_t tx_data[COMMAND_SIZE];
  uint8_t rx_data[COMMAND_SIZE];
  uint16_t data_buffer = 0;
  uint16_t encoder_angle;

  uint8_t data[128];  // Increased buffer size
  memset(data, '\0', sizeof(data));
  char text[100];

  /* Wait for the rotary encoder to finish its initialization (100 ms) */
  vTaskDelay(pdMS_TO_TICKS(100));

  /* Clear the transmit and receive buffers */
  memset(tx_data, 0, COMMAND_SIZE);
  memset(rx_data, 0, COMMAND_SIZE);

  /* Enter Infinite loop after connecting with the encoder */
  for(;;) {
    // Initial master command to read position
    tx_data[0] = 0x10;

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); // Set CS Low
    HAL_SPI_TransmitReceive(&hspi1, tx_data, rx_data, COMMAND_SIZE, CMD_TIMEOUT);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET); // Set CS High

    /* If data buffer is non-zero, this implies that data was successfully captured from the previous
       read command. This means the current read buffer has LSB data which needs to be further processed. Note,
       this is equivalent to sending the second NOP command as per the datasheet. */
    if (data_buffer != 0) {
      // Retrieve LSB Position from current rx_data buffer (data from previous read command)
      data_buffer |= rx_data[0];

      // Map sensor reading into an angle for processing
      encoder_angle = (data_buffer / 4095.0) * 360.0;

      // Output sensor reading
      snprintf((char *)data, sizeof(data), "%d\r\n", encoder_angle);  // Use snprintf and cast data to char *
      HAL_UART_Transmit(&huart3, data, strlen((char *)data), HAL_MAX_DELAY);  // Cast data to char *
    }

    while(1) {
      static uint8_t attempts = 0;

      // Continue sending NOP command until data is available
      tx_data[0] = 0x00;

      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); // CS Low
      HAL_SPI_TransmitReceive(&hspi1, tx_data, rx_data, COMMAND_SIZE, CMD_TIMEOUT);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET); // CS High

      // Check if rx_data buffer is equal to the original command that was issued (read == 0x10)
      if (rx_data[0] == 0x10) {
        HAL_UART_Transmit(&huart3, (uint8_t *)"Data!\r\n", 7, HAL_MAX_DELAY);  // Cast to uint8_t *
        break;
      }

      /* Send NOP commands three times to ensure the next time read command is issued, 
         the encoder returns the same command back properly. */
      else if (rx_data[0] == 0xA5) {
        attempts++;
        if (attempts == 3) {
          break;
        }
      }

      else {
        // Print current rx_data buffer value
        snprintf((char *)data, sizeof(data), "%u\r\n", rx_data[0]);  // Use snprintf and cast data to char *
        HAL_UART_Transmit(&huart3, data, strlen((char *)data), HAL_MAX_DELAY);  // Cast data to char *
      }

      vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    if (rx_data[0] == 0x10) {
      // Clear data buffer prior to read
      data_buffer = 0;

      // First NOP command after issuing a successful read command
      tx_data[0] = 0x00;
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); // Set CS Low
      if (HAL_SPI_TransmitReceive(&hspi1, tx_data, rx_data, COMMAND_SIZE, CMD_TIMEOUT) != HAL_OK) {
        strcpy(text, "Unable to send NOP Command to the sensor ");
        snprintf((char *)data, sizeof(data), "%s\r\n", text);  // Use snprintf and cast data to char *
        HAL_UART_Transmit(&huart3, data, strlen((char *)data), HAL_MAX_DELAY);  // Cast data to char *
      }
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET); // Set CS High

      vTaskDelay(pdMS_TO_TICKS(1));

      // Retrieve MSB position
      data_buffer = (rx_data[0] & 0xF) << 8;
    }
  
  /* USER CODE END 5 */
}
}
