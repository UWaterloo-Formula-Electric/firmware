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
osThreadId mainTaskHandle;
osThreadId rtryEncTaskNameHandle;
osThreadId brkIRTaskNameHandle;
osThreadId halEfSensNameHandle;
osThreadId wtrTempTaskNameHandle;
osThreadId watchdogTaskNamHandle;
osThreadId printTaskNameHandle;
osThreadId cliTaskNameHandle;
osThreadId canSendTaskHandle;
osThreadId canLogTaskNameHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void mainTaskFunction(void const * argument);
extern void RotaryEncoderTask(void const * argument);
extern void BrakeIRTask(void const * argument);
extern void HallEffectSensorTask(void const * argument);
extern void WaterflowTempSensorTask(void const * argument);
extern void watchdogTask(void const * argument);
extern void printTask(void const * argument);
extern void cliTask(void const * argument);
extern void canTask(void const * argument);
extern void canLogTask(void const * argument);

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
  /* definition and creation of mainTask */
  osThreadDef(mainTask, mainTaskFunction, osPriorityNormal, 0, 256);
  mainTaskHandle = osThreadCreate(osThread(mainTask), NULL);

  /* definition and creation of rtryEncTaskName */
  osThreadDef(rtryEncTaskName, RotaryEncoderTask, osPriorityLow, 0, 128);
  rtryEncTaskNameHandle = osThreadCreate(osThread(rtryEncTaskName), NULL);

  /* definition and creation of brkIRTaskName */
  osThreadDef(brkIRTaskName, BrakeIRTask, osPriorityLow, 0, 512);
  brkIRTaskNameHandle = osThreadCreate(osThread(brkIRTaskName), NULL);

  /* definition and creation of halEfSensName */
  osThreadDef(halEfSensName, HallEffectSensorTask, osPriorityLow, 0, 256);
  halEfSensNameHandle = osThreadCreate(osThread(halEfSensName), NULL);

  /* definition and creation of wtrTempTaskName */
  osThreadDef(wtrTempTaskName, WaterflowTempSensorTask, osPriorityNormal, 0, 1024);
  wtrTempTaskNameHandle = osThreadCreate(osThread(wtrTempTaskName), NULL);

  /* definition and creation of watchdogTaskNam */
  osThreadDef(watchdogTaskNam, watchdogTask, osPriorityRealtime, 0, 160);
  watchdogTaskNamHandle = osThreadCreate(osThread(watchdogTaskNam), NULL);

  /* definition and creation of printTaskName */
  osThreadDef(printTaskName, printTask, osPriorityLow, 0, 128);
  printTaskNameHandle = osThreadCreate(osThread(printTaskName), NULL);

  /* definition and creation of cliTaskName */
  osThreadDef(cliTaskName, cliTask, osPriorityLow, 0, 256);
  cliTaskNameHandle = osThreadCreate(osThread(cliTaskName), NULL);

  /* definition and creation of canSendTask */
  osThreadDef(canSendTask, canTask, osPriorityRealtime, 0, 256);
  canSendTaskHandle = osThreadCreate(osThread(canSendTask), NULL);

  /* definition and creation of canLogTaskName */
  osThreadDef(canLogTaskName, canLogTask, osPriorityRealtime, 0, 2048);
  canLogTaskNameHandle = osThreadCreate(osThread(canLogTaskName), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_mainTaskFunction */
/**
  * @brief  Function implementing the mainTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_mainTaskFunction */
__weak void mainTaskFunction(void const * argument)
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
