#include <stdint.h>
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
	f_mount(&fs, "", 1);
    f_open(&fil, "LOG.txt", FA_WRITE | FA_OPEN_APPEND | FA_READ);

    while (1)
    {
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