/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
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
osThreadId mainTaskHandle;
osThreadId printTaskNameHandle;
osThreadId cliTaskNameHandle;
osThreadId controlTaskNameHandle;
osThreadId PCDCHandle;
osThreadId BatteryTaskHandle;
osThreadId sensorTaskNameHandle;
osThreadId watchdogTaskNamHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void mainTaskFunction(void const * argument);
extern void printTask(void const * argument);
extern void cliTask(void const * argument);
extern void controlTask(void const * argument);
extern void pcdcTask(void const * argument);
extern void batteryTask(void const * argument);
extern void sensorTask(void const * argument);
extern void watchdogTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

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
  osThreadDef(mainTask, mainTaskFunction, osPriorityNormal, 0, 1000);
  mainTaskHandle = osThreadCreate(osThread(mainTask), NULL);

  /* definition and creation of printTaskName */
  osThreadDef(printTaskName, printTask, osPriorityLow, 0, 1000);
  printTaskNameHandle = osThreadCreate(osThread(printTaskName), NULL);

  /* definition and creation of cliTaskName */
  osThreadDef(cliTaskName, cliTask, osPriorityLow, 0, 1000);
  cliTaskNameHandle = osThreadCreate(osThread(cliTaskName), NULL);

  /* definition and creation of controlTaskName */
  osThreadDef(controlTaskName, controlTask, osPriorityNormal, 0, 1000);
  controlTaskNameHandle = osThreadCreate(osThread(controlTaskName), NULL);

  /* definition and creation of PCDC */
  osThreadDef(PCDC, pcdcTask, osPriorityNormal, 0, 1000);
  PCDCHandle = osThreadCreate(osThread(PCDC), NULL);

  /* definition and creation of BatteryTask */
  osThreadDef(BatteryTask, batteryTask, osPriorityRealtime, 0, 1000);
  BatteryTaskHandle = osThreadCreate(osThread(BatteryTask), NULL);

  /* definition and creation of sensorTaskName */
  osThreadDef(sensorTaskName, sensorTask, osPriorityNormal, 0, 1000);
  sensorTaskNameHandle = osThreadCreate(osThread(sensorTaskName), NULL);

  /* definition and creation of watchdogTaskNam */
  osThreadDef(watchdogTaskNam, watchdogTask, osPriorityRealtime, 0, 1000);
  watchdogTaskNamHandle = osThreadCreate(osThread(watchdogTaskNam), NULL);

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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
