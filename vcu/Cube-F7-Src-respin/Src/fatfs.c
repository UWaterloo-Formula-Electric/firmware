/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file   fatfs.c
  * @brief  Code for fatfs applications
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
#include "rtc.h"
/* USER CODE END Header */
#include "fatfs.h"

uint8_t retUSER;    /* Return value for USER */
char USERPath[4];   /* USER logical drive path */
FATFS USERFatFS;    /* File system object for USER logical drive */
FIL USERFile;       /* File object for USER */

/* USER CODE BEGIN Variables */

/* USER CODE END Variables */

void MX_FATFS_Init(void)
{
  /*## FatFS: Link the USER driver ###########################*/
  retUSER = FATFS_LinkDriver(&USER_Driver, USERPath);

  /* USER CODE BEGIN Init */
  /* additional user code for init */
  /* USER CODE END Init */
}

/**
  * @brief  Gets Time from RTC
  * @param  None
  * @retval Time in DWORD
  */
DWORD get_fattime(void)
{
  /* USER CODE BEGIN get_fattime */
  RTC_TimeTypeDef sZeit;

  RTC_DateTypeDef sDatum;

  DWORD attime;

  HAL_RTC_GetTime(&hrtc, &sZeit, RTC_FORMAT_BIN);

  HAL_RTC_GetDate(&hrtc, &sDatum, RTC_FORMAT_BIN);

  attime = (((DWORD)sDatum.Year - 1980) << 25) |
           ((DWORD)sDatum.Month << 21) |
           ((DWORD)sDatum.Date << 16) |
           (DWORD)(sZeit.Hours << 11) |
           (DWORD)(sZeit.Minutes << 5) |
           (DWORD)(sZeit.Seconds >> 1);

  return attime;
}
  /* USER CODE END get_fattime */

/* USER CODE BEGIN Application */

/* USER CODE END Application */
