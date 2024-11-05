#include "canLogger.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "debug.h"
#include "fatfs.h"
#include "generalErrorHandler.h"
#include "sd_diskio.h"
#include "sdio.h"
#include "stm32f4xx_hal.h"
#include "task.h"

#define CAN_LOG_TASK_PERIOD 50  // any higher and we might lose logs, any lower and we start starving runtime from other tasks

#define LOG_SB_SIZE 500                        // max size of the stream buffer
#define STREAM_BUFFER_TRIGGER_LEVEL 10         // the reading task is unblocked after atleast 10 items are in the stream buffer
#define MAX_LOG_FILE_BYTES (10 * 1024 * 1024)  // 10MB, rotate to new file after this size
#define LOG_FILE_SYNC_BYTES (16 * 1024)        // 16kB
// start logging only after this variable is set to true
// it set to true if new log file is created successfully
volatile bool isCanLogEnabled = false;

FATFS FatFs;

FRESULT mountSD(FATFS *fs) {
    // SDPath auto provided by fatfs.h, do not edit it
    return f_mount(fs, SDPath, 1);
}

FRESULT unmountSD() {
    // SDPath auto provided by fatfs.h, do not edit it
    return f_mount(NULL, SDPath, 0);
}

/**
 * @brief Change the current directory to the specified path
 * @param path [IN] the path name of the directory to change to
 * @return error code from the fatfs library fn. calls, FR_OK if successful
 */
FRESULT chdir(const char *path) {
    FRESULT res = f_chdir(path);
    if (res != FR_OK) {
        ERROR_PRINT("Failed to change directory to %s with error %d\n", path, res);
    }
    return res;
}

/**
 * @brief Create a directory, if it already exists, do nothing
 * @param path [IN] the path name of the directory to create
 * @return error code from the fatfs library fn. calls, FR_OK if successful
 */
FRESULT mkdir(const char *path) {
    FRESULT res = f_mkdir(path);
    if (res != FR_OK && res != FR_EXIST) {
        ERROR_PRINT("Failed to create directory %s with error %d\n", path, res);
        return res;
    }
    return FR_OK;
}

/**
 * @brief Open a file for writing in append mode, create it if it doesn't exist
 * @param fp [OUT] File object to create
 * @param path [IN] File name to be opened
 * @return error code from the fatfs library fn. calls, FR_OK if successful
 */
FRESULT open_append(FIL *fp, const char *path) {
    FRESULT res;

    /* Opens an existing file. If not exist, creates a new file. */
    res = f_open(fp, path, FA_WRITE | FA_OPEN_ALWAYS);
    if (res == FR_OK) {
        /* Seek to end of the file to append data */
        res = f_lseek(fp, f_size(fp));
        if (res != FR_OK)
            f_close(fp);
    }
    return res;
}

/**
 * @brief Recursively print the contents of a directory and its subdirectories
 * @param path [IN] the path name of the directory
 * @param maxLevel [IN] the maximum depth of subdirectories to print
 */
void printDir(char *path, uint8_t maxLevel) {
    if (maxLevel == 0) {
        return;
    }

    DIR dir;
    FILINFO fno;
    FRESULT res;

    res = f_opendir(&dir, path);
    if (res != FR_OK) {
        ERROR_PRINT("Failed to open directory %s\n", path);
        return;
    }

    uint32_t fileCnt = 0;
    while (1) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) {
            break;
        }
        if (fno.fattrib & AM_DIR) {
            char dirPath[100];
            snprintf(dirPath, 100, "%s/%s", path, fno.fname);
            printDir(dirPath, maxLevel - 1);
        } else {
            fileCnt++;
            DEBUG_PRINT("%s/%s\n", path, fno.fname);
        }
    }
    if (fileCnt == 0) {
        DEBUG_PRINT("%s/\n", path);
    }
    f_closedir(&dir);
}

/**
 * @brief Create a new file with the next available file name in the format "LOGxxx.txt"
 * @param file [OUT] the file object to create
 * @param path [IN] the dir to search for the file
 * @param fileName [OUT] the buffer to store the file name
 * @param size [IN] the size of the buffer
 * @return error code from the fatfs library fn. calls, FR_OK if successful
 */
FRESULT createAndOpenNextFile(FIL *file, char *path, char *fileName, size_t size) {
    TCHAR cwd[100];
    FRESULT res = f_getcwd(cwd, 100);
    if (res != FR_OK) {
        ERROR_PRINT("Failed to get current directory with error %d\n", res);
        return res;
    }
    // compare cwd + 1 as by default it returns "/<dir>"
    // and for creeating dirs we can't use the leading "/"
    // so ignore the leading "/" in the comparison
    if (strncmp(path, cwd + 1, 100) != 0) {
        res = f_chdir(path);

        if (res != FR_OK) {
            ERROR_PRINT("Failed to change directory to %s with error %d\n", path, res);
            return res;
        }
    }

    // Find the next available file name
    FILINFO fno;
    static size_t fileidx = 0;
    do {
        snprintf(fileName, size, "LOGS%d.TXT", (int)fileidx);
        res = f_stat(fileName, &fno);
        fileidx++;
    } while (res != FR_NO_FILE);

    res = open_append(file, fileName);
    if (res != FR_OK) {
        ERROR_PRINT("Failed to open file %s with error %d\n", fileName, res);
    }

    return res;
}

/**
 * @brief Create a new directory with the next available dir name in the format "LOGSxxx"
 * @param path [IN] the dir to search for the dir
 * @param dirName [OUT] the buffer to store the dir name
 * @param size [IN] the size of the buffer
 * @return error code from the fatfs library fn. calls, FR_OK if successful
 */
FRESULT createNextDir(char *path, char *dirName, size_t size) {
    TCHAR cwd[100];
    FRESULT res = f_getcwd(cwd, 100);
    if (res != FR_OK) {
        ERROR_PRINT("Failed to get current directory with error %d\n", res);
        return res;
    }
    // compare cwd + 1 as by default it returns "/<dir>"
    // and for creeating dirs we can't use the leading "/"
    // so ignore the leading "/" in the comparison
    if (strncmp(path, cwd + 1, 100) != 0) {
        res = f_chdir(path);

        if (res != FR_OK) {
            ERROR_PRINT("Failed to change directory to %s with error %d\n", path, res);
            return res;
        }
    }

    // Find the next available dir name
    FILINFO fno;
    static size_t diridx = 0;
    do {
        snprintf(dirName, size, "LOGS%d", (int)diridx);
        res = f_stat(dirName, &fno);
        diridx++;
    } while (res != FR_NO_FILE);

    res = f_mkdir(dirName);
    if (res != FR_OK && res != FR_EXIST) {
        ERROR_PRINT("Failed to create directory %s with error %d\n", dirName, res);
    }

    return res;
}

/**
 * @brief Initialize the SD card and mount it, must be called after RTOS scheduler is running
 */
FRESULT initCANLoggerSD() {
    FRESULT FR_status;

    FR_status = mountSD(&FatFs);
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
    DEBUG_PRINT("SD Card Total Size: %luMB, Free Space: %luMB\n", TotalSize, FreeSpace);
    return FR_status;
}

StaticStreamBuffer_t canLogSB_struct;
StreamBufferHandle_t canLogSB = NULL;

CanMsg canLogBuffer[LOG_SB_SIZE + 1];  // stream buffer requires 1 extra byte

HAL_StatusTypeDef canLogSB_init() {
    // init a static buffer for the can log stream buffer, call in user init
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

    // Find and create new file
    char fileName[15];
    FIL file;
    FRESULT res;

    isCanLogEnabled = false;

    char logDir[100] = "\0";
    res = createNextDir(SDPath, logDir, 15);
    if (res != FR_OK && res != FR_EXIST) {
        ERROR_PRINT("Failed to create directory %s with error %d\n", logDir, res);
        handleError();
    }

    // upto 100 messages can be read from buffer at a time
    // (this is arbitrary but, I measured @ 10ms task period, 100% CAN bus load, max #can msgs was ~50)
    // so 2x safety factor should be enough
    CanMsg rxMsgs[200];
    size_t writtenBytes = 0;
    int sentBytes = 0;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1) {
        // Try to open the next file
        if (!isCanLogEnabled) {
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CAN_LOG_TASK_PERIOD));
            res = createAndOpenNextFile(&file, logDir, fileName, 15);
            isCanLogEnabled = res == FR_OK;
            if (isCanLogEnabled) {
                DEBUG_PRINT("Opened new log file %s/%s\n", logDir, fileName);
            } else {
                ERROR_PRINT("Failed to open file %s/%s with error %d\n", logDir, fileName, res);
            }
        }

        // Tested upto 99.9% bus load
        size_t readBytes = xStreamBufferReceive(canLogSB, rxMsgs, sizeof(rxMsgs), portMAX_DELAY);
        size_t numReadMsgs = readBytes / sizeof(CanMsg);
        for (size_t i = 0; i < numReadMsgs; i++) {
            CanMsg rxMsg = rxMsgs[i];
            // append to file, note this doesn't acc write to sd card until f_sync is called
            sentBytes = f_printf(&file, "%08lXx%08lX%02X%02X%02X%02X%02X%02X%02X%02X\n", xTaskGetTickCount(), rxMsg.id, rxMsg.data[0], rxMsg.data[1], rxMsg.data[2], rxMsg.data[3], rxMsg.data[4], rxMsg.data[5], rxMsg.data[6], rxMsg.data[7]);
            if (sentBytes < 0) {
                ERROR_PRINT("Failed to write to file with error %d\n", sentBytes);
            }
            if (failedFifoCount > 0)
                failedFifoCount--;
        }

        // sync every 16kB, a sync will commit the data to the sd card
        writtenBytes += sentBytes;
        if (writtenBytes >= LOG_FILE_SYNC_BYTES) {
            f_sync(&file);
            // DEBUG_PRINT("Synced\n");
            writtenBytes = 0;
        }

        // close file if it's too big
        if (f_size(&file) > MAX_LOG_FILE_BYTES) {
            f_close(&file);  // close also syncs the file
            writtenBytes = 0;
            isCanLogEnabled = false;
        }

        // uint32_t end = xTaskGetTickCount();
        // if (xTaskGetTickCount() % 1000 == 0) {
        // DEBUG_PRINT("SB: %u FF: %lu %ld\n", xStreamBufferBytesAvailable(canLogSB) / sizeof(CanMsg), failedFifoCount, end - start);
        // }
        // print size of stream buffer

        // DEBUG_PRINT("%08lx: %0x %0x %0x %0x %0x %0x %0x %0x\n", rxMsg.id, rxMsg.data[0], rxMsg.data[1], rxMsg.data[2], rxMsg.data[3], rxMsg.data[4], rxMsg.data[5], rxMsg.data[6], rxMsg.data[7]);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CAN_LOG_TASK_PERIOD));
    }
}