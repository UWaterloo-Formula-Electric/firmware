/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId rotaryEncoderHandle;
osThreadId brakeInfraredHandle;
osThreadId hallEffSensorHandle;
osThreadId waterTempSensorHandle;
osThreadId watchdogTaskNamHandle;
osThreadId mainTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartRotaryEncoderTask(void const * argument);
void BrakeIRTask(void const * argument);
void StartHallEffectSensorTask(void const * argument);
void StartWaterflowTempSensorTask(void const * argument);
void watchdogTask(void const * argument);
void mainTaskFunction(void const * argument);

extern void MX_USB_HOST_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{

}

__weak unsigned long getRunTimeCounterValue(void)
{
return 0;
}
/* USER CODE END 1 */

/* USER CODE BEGIN 4 */
__weak void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
}
/* USER CODE END 4 */

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of rotaryEncoder */
  osThreadDef(rotaryEncoder, StartRotaryEncoderTask, osPriorityNormal, 0, 128);
  rotaryEncoderHandle = osThreadCreate(osThread(rotaryEncoder), NULL);

  /* definition and creation of brakeInfrared */
  osThreadDef(brakeInfrared, BrakeIRTask, osPriorityNormal, 0, 512);
  brakeInfraredHandle = osThreadCreate(osThread(brakeInfrared), NULL);

  /* definition and creation of hallEffSensor */
  osThreadDef(hallEffSensor, StartHallEffectSensorTask, osPriorityNormal, 0, 512);
  hallEffSensorHandle = osThreadCreate(osThread(hallEffSensor), NULL);

  /* definition and creation of waterTempSensor */
  osThreadDef(waterTempSensor, StartWaterflowTempSensorTask, osPriorityNormal, 0, 1024);
  waterTempSensorHandle = osThreadCreate(osThread(waterTempSensor), NULL);

  /* definition and creation of watchdogTaskNam */
  osThreadDef(watchdogTaskNam, watchdogTask, osPriorityRealtime, 0, 160);
  watchdogTaskNamHandle = osThreadCreate(osThread(watchdogTaskNam), NULL);

  /* definition and creation of mainTask */
  osThreadDef(mainTask, mainTaskFunction, osPriorityHigh, 0, 256);
  mainTaskHandle = osThreadCreate(osThread(mainTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for USB_HOST */
  MX_USB_HOST_Init();
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartRotaryEncoderTask */
/**
* @brief Function implementing the rotaryEncoder thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartRotaryEncoderTask */
void StartRotaryEncoderTask(void const * argument)
{
  /* USER CODE BEGIN StartRotaryEncoderTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartRotaryEncoderTask */
}

/* USER CODE BEGIN Header_BrakeIRTask */
/**
* @brief Function implementing the brakeInfrared thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_BrakeIRTask */
void BrakeIRTask(void const * argument)
{
  /* USER CODE BEGIN BrakeIRTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END BrakeIRTask */
}

/* USER CODE BEGIN Header_StartHallEffectSensorTask */
/**
* @brief Function implementing the hallEffSensor thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartHallEffectSensorTask */
void StartHallEffectSensorTask(void const * argument)
{
  /* USER CODE BEGIN StartHallEffectSensorTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartHallEffectSensorTask */
}

/* USER CODE BEGIN Header_StartWaterflowTempSensorTask */
/**
* @brief Function implementing the waterTempSensor thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartWaterflowTempSensorTask */
void StartWaterflowTempSensorTask(void const * argument)
{
  /* USER CODE BEGIN StartWaterflowTempSensorTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartWaterflowTempSensorTask */
}

/* USER CODE BEGIN Header_watchdogTask */
/**
* @brief Function implementing the watchdogTaskNam thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_watchdogTask */
void watchdogTask(void const * argument)
{
  /* USER CODE BEGIN watchdogTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END watchdogTask */
}

/* USER CODE BEGIN Header_mainTaskFunction */
/**
* @brief Function implementing the mainTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_mainTaskFunction */
void mainTaskFunction(void const * argument)
{
  /* USER CODE BEGIN mainTaskFunction */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END mainTaskFunction */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
