#include <stdio.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "debug.h"
#include "fatfs.h"
#include "generalErrorHandler.h"
#include "sd_diskio.h"
#include "sdio.h"
#include "stm32f4xx_hal.h"
#include "task.h"
#include "watchdog.h"

FRESULT mountSD(FATFS *fs) {
    // SDPath auto provided by fatfs.h, do not edit it
    return f_mount(fs, SDPath, 1);
}

FRESULT unmountSD() {
    return f_mount(NULL, SDPath, 0);
}

FATFS FatFs;
/**
 * @brief Initialize the SD card and mount it, must be called after RTOS scheduler is running
 */
FRESULT initCANLoggerSD() {
    FRESULT FR_status;

    watchdogRefresh();
    FR_status = f_mount(&FatFs, SDPath, 1);
    watchdogRefresh();
    if (FR_status != FR_OK) {
        ERROR_PRINT("Error mounting SD card with FRESULT: %d\n", FR_status);
        return FR_status;
    }
    DEBUG_PRINT("Mounted SD card on drive %s\n", SDPath);

    FATFS *FS_Ptr;
    DWORD FreeClusters;
    uint32_t TotalSize, FreeSpace;

    FR_status = f_getfree("", &FreeClusters, &FS_Ptr);
    if (FR_status != FR_OK) {
        ERROR_PRINT("Error getting freespace SD card with FRESULT: %d\n", FR_status);
        return FR_status;
    }
    TotalSize = (FS_Ptr->n_fatent - 2) * FS_Ptr->csize / 2 / 1024;
    FreeSpace = FreeClusters * FS_Ptr->csize / 2 / 1024;
    DEBUG_PRINT("SD Card Total Size: %lukB, Free Space: %lukB\n", TotalSize, FreeSpace);

    return FR_status;
}

void canLogTask(void *arg) {
    DEBUG_PRINT("Deleting CANLogTask\n");
    vTaskDelete(NULL);
    DEBUG_PRINT("Starting CAN Logger\n");
    // if (initCANLoggerSD() != FR_OK) {
    //     handleError();
    // }
    while (1) {
        vTaskDelay(10000);
    }
}