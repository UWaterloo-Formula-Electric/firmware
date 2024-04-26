#include <stdint.h>
#include <string.h>
#include "sd_logging.h"
#include "debug.h"
#include "task.h"
#include "FreeRTOS.h"
#include "can.h"
#include "fatfs.h"

#define SD_LOG_TASK_PERIOD 1

QueueHandle_t sdLoggingQueue;

HAL_StatusTypeDef initSdLoggingQueue(void)
{
    sdLoggingQueue = xQueueCreate(50,sizeof(can_msg_t));
    if (sdLoggingQueue == NULL) {
      ERROR_PRINT("Failed to create sd logging queue!\n");
      return HAL_ERROR;
   }

   return HAL_OK;
}

void sdLoggingTask(void)
{
    FATFS fs;
    FIL fil;
    can_msg_t logMsg;

    char fileExt[16] = "-LOG.txt";
    char fileName[64];
    sprintf(fileName, "%ld", get_fattime());
    strcat(fileName, fileExt);

    f_mount(&fs, "", 1);
    f_open(&fil, fileName, FA_WRITE | FA_OPEN_APPEND | FA_READ);
    f_printf(&fil, "%d", get_fattime()); // log time for the first line

    while (1)
    {
        // in the CAN callback function, CAN msgs are sent to the queue
        xQueueReceive(sdLoggingQueue,&logMsg, portMAX_DELAY);
        f_printf(&fil, "%d ", HAL_GetTick());
        f_printf(&fil, "%d", logMsg.id);
        for(int i = 0; i < 8; i++) {
            f_printf(&fil, "%d ", logMsg.data[i]);
        }
        f_printf(&fil, "\r\n");
        f_sync(&fil);
        vTaskDelay(SD_LOG_TASK_PERIOD);
    }

}