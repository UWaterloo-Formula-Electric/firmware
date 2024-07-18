#include "main.h"
#include "cmsis_os.h"
#include "stdio.h"
#include "stm32f4xx_hal_tim.h"

TIM_HandleTypeDef htim3;
UART_HandleTypeDef huart2;
uint32_t counter = 0;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM3_Init(void);
void StartHallEffectSensorTask(void const * argument);

//redirect printf to uart
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
PUTCHAR_PROTOTYPE
{
HAL_UART_Transmit(&huart2 , (uint8_t *)&ch, 1 , 0xffff);
return ch;
}
/* USER CODE END 0 */

void StartHallEffectSensorTask(void const * argument)
{
  /* USER CODE BEGIN 5 */
	char msg[64];
	sprintf(msg, "Val = %d\r\n", counter);
	uint32_t timer_val;
//	HAL_TIM_Base_Start(&htim3);
	  /* Infinite loop */
	  for(;;)
	  {
	//	  timer_val = __HAL_TIM_GET_COUNTER(&htim3);

	//	  timer_val = __HAL_TIM_GET_COUNTER(&htim3) - timer_val;
//		  printf("%d",counter);
	//	  printf("val=%d",(counter / timer_val)); // How to output value?
		  printf("Val = %d\r\n", counter);
		  osDelay(500);
	  }
  /* USER CODE END 5 */
}

