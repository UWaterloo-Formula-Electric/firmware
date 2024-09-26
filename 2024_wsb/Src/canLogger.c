#include "canLogger.h"
#include "watchdog.h"
#include <stdio.h>

#include "FreeRTOS.h"
#include "debug.h"
#include "fatfs.h"
#include "generalErrorHandler.h"
#include "sdio.h"
#include "stm32f4xx_hal.h"
#include "task.h"
#include "sd_diskio.h"

FRESULT mountSD(FATFS *fs) {
    // SDPath auto provided by fatfs.h, do not edit it
    return  f_mount(fs, SDPath, 1);
}

FRESULT unmountSD() {
    return f_mount(NULL, SDPath, 0);
}

FATFS FatFs;
HAL_StatusTypeDef initCANLoggerSD() {
    
    // FRESULT FR_status;

    // // // printf("Drive Path: [%s]\n", SDPath);
    // // // HAL_Delay(1000);
    // FR_status = f_mount(&FatFs, SDPath, 0);
    // if (FR_status != FR_OK) {
    //     printf("Error mounting SD card with FRESULT: %d\n", FR_status);
    //     return HAL_ERROR;
    // }
    // printf("Delay mounted SD card!\n");

    // FATFS *FS_Ptr;
    // DWORD FreeClusters;
    // uint32_t TotalSize, FreeSpace;

    // FR_status = f_getfree("", &FreeClusters, &FS_Ptr);
    // if (FR_status != FR_OK) {
    //     printf("Error getting freespace SD card with FRESULT: %d\n", FR_status);
    //     return HAL_ERROR;
    // }
    // TotalSize = (uint32_t)((FS_Ptr->n_fatent - 2) * FS_Ptr->csize * 0.5);
    // FreeSpace = (uint32_t)(FreeClusters * FS_Ptr->csize * 0.5);
    // printf("SD Card Total Size: %luMB, Free Space: %luMB\n", TotalSize, FreeSpace);

    return HAL_OK;
}

void canLogTask(void *arg) {
    DEBUG_PRINT("Starting CAN Logger\n");
    // uint32_t start = HAL_GetTick();
    // vTaskDelay(WAIT_FOR_SD_PRE_INIT_MS);
    watchdogRefresh();

    FRESULT FR_status = f_mount(&FatFs, SDPath, 1);
    watchdogRefresh();
    // uint32_t end = HAL_GetTick();
    if (FR_status != FR_OK) {
        DEBUG_PRINT("Error mounting SD card with FRESULT: %d\n", FR_status);
        handleError();
    }
    DEBUG_PRINT("Mounted SD card!\n");
    // DEBUG_PRINT("Time to mount: %lu\n", end - start);
    while (1) {
        vTaskDelay(10);
    }
}