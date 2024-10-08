#include "canLogger.h"

#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "debug.h"
#include "fatfs.h"
#include "generalErrorHandler.h"
#include "sd_diskio.h"
#include "sdio.h"
#include "stm32f4xx_hal.h"
#include "task.h"
#include "watchdog.h"

#define CAN_LOGGER_TASK_PERIOD 1000
#define BITRATE 500000
#define CAN_OVERHEAD_EXT 66  // overhead for ext can msg with 0 data see https://en.wikipedia.org/wiki/CAN_bus#Extended_frame_format

// welp we ran out of ram, 1000 seems to be the max we can do
#define LOG_SB_SIZE 500  // (BITRATE / CAN_OVERHEAD_EXT + 1)  // 1 second of data at max rate
#define STREAM_BUFFER_TRIGGER_LEVEL ((BaseType_t)10)
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
    FR_status = mountSD(&FatFs);
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

    // list files in root directory
    // DIR dir;               // Directory
    // FILINFO fno;           // File Info
    // f_opendir(&dir, SDPath);  // Open Root
    // do {
    //     f_readdir(&dir, &fno);
    //     if (fno.fname[0] != 0)
    //         DEBUG_PRINT("File found: %s\n", fno.fname);  // Print File Name
    // } while (fno.fname[0] != 0);

    // f_closedir(&dir);

    return FR_status;
}

StaticStreamBuffer_t canLogSB_struct;
StreamBufferHandle_t canLogSB = NULL;

CanMsg canLogBuffer[LOG_SB_SIZE + 1];

HAL_StatusTypeDef canLogSB_init() {
    canLogSB = xStreamBufferCreateStatic(sizeof(canLogBuffer), STREAM_BUFFER_TRIGGER_LEVEL, (uint8_t *)&canLogBuffer, &canLogSB_struct);
    if (!canLogSB) {
        return HAL_ERROR;
    }
    return HAL_OK;
}
extern volatile uint32_t failedFifoCount;
void canLogTask(void *arg) {
    DEBUG_PRINT("Starting CAN Logger Task\n");
    if (initCANLoggerSD() != FR_OK) {
        handleError();
    }
    CanMsg rxMsgs[100];

    while (1) {
        // Tested upto 100% bus load and it works
        DEBUG_PRINT("Stream buffer size: %d\n", xStreamBufferBytesAvailable(canLogSB) / sizeof(CanMsg));
        xStreamBufferReceive(canLogSB, rxMsgs, sizeof(rxMsgs), portMAX_DELAY);
        // print size of stream buffer
        
        // DEBUG_PRINT("Read bytes: %d\n", readBytes);
        // DEBUG_PRINT("Failed fifo count: %lu\n", failedFifoCount);
        // DEBUG_PRINT("%08lx: %0x %0x %0x %0x %0x %0x %0x %0x\n", rxMsg.id, rxMsg.data[0], rxMsg.data[1], rxMsg.data[2], rxMsg.data[3], rxMsg.data[4], rxMsg.data[5], rxMsg.data[6], rxMsg.data[7]);
        vTaskDelay(10);
    }
}