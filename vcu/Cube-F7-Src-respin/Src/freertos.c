/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
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
#include "stm32f7xx_hal.h"

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
osThreadId driveByWireHandle;
osThreadId mainTaskHandle;
osThreadId printTaskNameHandle;
osThreadId cliTaskNameHandle;
osThreadId watchdogTaskNamHandle;
osThreadId canSendTaskHandle;
osThreadId canPublishHandle;
osThreadId enduranceModeHandle;
osThreadId tractionControlHandle;
osThreadId throttlePollingHandle;
osThreadId ledHandle;
osThreadId buttonTaskNameHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void driveByWireTask(void const * argument);
extern void mainTaskFunction(void const * argument);
extern void printTask(void const * argument);
extern void cliTask(void const * argument);
extern void watchdogTask(void const * argument);
extern void canTask(void const * argument);
extern void canPublishTask(void const * argument);
extern void enduranceModeTask(void const * argument);
extern void tractionControlTask(void const * argument);
extern void throttlePollingTask(void const * argument);
extern void ledTask(void const * argument);
extern void buttonTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* GetTimerTaskMemory prototype (linked to static allocation support) */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize );

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

/* USER CODE BEGIN GET_TIMER_TASK_MEMORY */
static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t xTimerStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
  *ppxTimerTaskStackBuffer = &xTimerStack[0];
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
  /* place for user code */
}
/* USER CODE END GET_TIMER_TASK_MEMORY */

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
  /* definition and creation of driveByWire */
  osThreadDef(driveByWire, driveByWireTask, osPriorityRealtime, 0, 1000);
  driveByWireHandle = osThreadCreate(osThread(driveByWire), NULL);

  /* definition and creation of mainTask */
  osThreadDef(mainTask, mainTaskFunction, osPriorityNormal, 0, 1000);
  mainTaskHandle = osThreadCreate(osThread(mainTask), NULL);

  /* definition and creation of printTaskName */
  osThreadDef(printTaskName, printTask, osPriorityLow, 0, 1000);
  printTaskNameHandle = osThreadCreate(osThread(printTaskName), NULL);

  /* definition and creation of cliTaskName */
  osThreadDef(cliTaskName, cliTask, osPriorityLow, 0, 1000);
  cliTaskNameHandle = osThreadCreate(osThread(cliTaskName), NULL);

  /* definition and creation of watchdogTaskNam */
  osThreadDef(watchdogTaskNam, watchdogTask, osPriorityRealtime, 0, 1000);
  watchdogTaskNamHandle = osThreadCreate(osThread(watchdogTaskNam), NULL);

  /* definition and creation of canSendTask */
  osThreadDef(canSendTask, canTask, osPriorityRealtime, 0, 1000);
  canSendTaskHandle = osThreadCreate(osThread(canSendTask), NULL);

  /* definition and creation of canPublish */
  osThreadDef(canPublish, canPublishTask, osPriorityNormal, 0, 1000);
  canPublishHandle = osThreadCreate(osThread(canPublish), NULL);

  /* definition and creation of enduranceMode */
  osThreadDef(enduranceMode, enduranceModeTask, osPriorityNormal, 0, 10000);
  enduranceModeHandle = osThreadCreate(osThread(enduranceMode), NULL);

  /* definition and creation of tractionControl */
  osThreadDef(tractionControl, tractionControlTask, osPriorityNormal, 0, 1024);
  tractionControlHandle = osThreadCreate(osThread(tractionControl), NULL);

  /* definition and creation of throttlePolling */
  osThreadDef(throttlePolling, throttlePollingTask, osPriorityRealtime, 0, 1000);
  throttlePollingHandle = osThreadCreate(osThread(throttlePolling), NULL);

  /* definition and creation of led */
  osThreadDef(led, ledTask, osPriorityNormal, 0, 128);
  ledHandle = osThreadCreate(osThread(led), NULL);

  /* definition and creation of buttonTaskName */
  osThreadDef(buttonTaskName, buttonTask, osPriorityAboveNormal, 0, 128);
  buttonTaskNameHandle = osThreadCreate(osThread(buttonTaskName), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_driveByWireTask */
/**
  * @brief  Function implementing the driveByWire thread.
  * @param  argument: Not used 
  * @retval None
  */
/* USER CODE END Header_driveByWireTask */
__weak void driveByWireTask(void const * argument)
{
  /* USER CODE BEGIN driveByWireTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END driveByWireTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
     
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
