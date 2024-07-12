#include "main.h"
#include "cmsis_os.h"

ADC_HandleTypeDef hadc;

UART_HandleTypeDef huart2;

void brakeIRTask(void const * argument);

void brakeIRTask(void const * argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  uint16_t raw;
  float voltage;
  float temp;
  char msg_temp[30];
  char msg_voltage[30];
  HAL_ADC_Start(&hadc);
  
  /* Infinite loop */
  for(;;)
  {
    HAL_ADC_Start(&hadc);
    HAL_ADC_PollForConversion(&hadc, HAL_MAX_DELAY);
    raw = HAL_ADC_GetValue(&hadc);
    
    voltage = ((float) raw) / (4095.0f) * 3.3;
    temp = ((voltage / 3) * 450) - 70;
        
    sprintf(msg_voltage, "Voltage: %f\r\n", voltage);
    HAL_UART_Transmit(&huart2, (uint8_t*)msg_voltage, strlen(msg_voltage), HAL_MAX_DELAY);
    
    sprintf(msg_temp, "Temp: %.2f\r\n", temp);
    HAL_UART_Transmit(&huart2, (uint8_t*)msg_temp, strlen(msg_temp), HAL_MAX_DELAY);
    
    osDelay(300);
  }
  /* USER CODE END 5 */
}