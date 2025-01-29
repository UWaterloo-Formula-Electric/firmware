#include "detectWSB.h"

#include <string.h>

#include "FreeRTOS.h"
#include "debug.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include "task.h"

/**
 * @brief Detects the WSB type based on the front/back & left/right DIP switches
 *        The WSB type is stored in a static variable to prevent re-detecting
 *        and switching mid-run
 */
WSBType_t detectWSB() {
    static WSBType_t lastDetectedWSB = INVALID_WSB;
    // prevent re-detecting the and swithcing mid-run
    if (lastDetectedWSB != INVALID_WSB) {
        return lastDetectedWSB;
    }

    GPIO_PinState front = HAL_GPIO_ReadPin(FrontRear_DipSW_GPIO_Port, FrontRear_DipSW_Pin);
    GPIO_PinState left = HAL_GPIO_ReadPin(LeftRight_DipSW_GPIO_Port, LeftRight_DipSW_Pin);

    if (front == GPIO_PIN_SET) {
        return lastDetectedWSB = (left == GPIO_PIN_SET ? WSBFL : WSBFR);
    } else {
        return lastDetectedWSB = (left == GPIO_PIN_SET ? WSBRL : WSBRR);
    }
}

/**
 * @brief Gets the board name based on the detected WSB type
 * @param boardName The buffer to store the board name (min 10 characters)
 */
bool getWSBBoardName(char* boardName, size_t size) {
    const size_t minSize = 10;
    if (size < minSize) {
        return false;
    }

    switch (detectWSB()) {
        case WSBFL:
            strncpy(boardName, "WSBFL", size);
            break;
        case WSBFR:
            strncpy(boardName, "WSBFR", size);
            break;
        case WSBRL:
            strncpy(boardName, "WSBRL", size);
            break;
        case WSBRR:
            strncpy(boardName, "WSBRR", size);
            break;
        default:
            strncpy(boardName, "INVALID", size);
            break;
    }
    return true;
}

/**
 * @brief Deletes the task if the WSB is not valid, else does nothing. Only call this function inside a task
 * @param validWSBs The valid WSBs for the task
 *
 * Ex: deleteWSBTaskIfNot(WSBFL | WSBFR); -> will delete the task if the WSB is not FL or FR
 * @return If the task was deleted or not
 */
bool deleteWSBTaskIfNot(uint8_t validWSBs) {
    WSBType_t wsbType = detectWSB();
    // no matching WSB, delete task
    if ((wsbType & validWSBs) == 0) {
        // put names of valid wsbs in str
        char validNames[50] = {0};
        int pos = 0;
        if (validWSBs & WSBFL)
            pos += snprintf(validNames + pos, sizeof(validNames), "WSBFL, ");
        if (validWSBs & WSBFR)
            pos += snprintf(validNames + pos, sizeof(validNames), "WSBFR, ");
        if (validWSBs & WSBRL)
            pos += snprintf(validNames + pos, sizeof(validNames), "WSBRL, ");
        if (validWSBs & WSBRR)
            pos += snprintf(validNames + pos, sizeof(validNames), "WSBRR, ");
        validNames[pos - 2] = '\0';  // remove last comma
        char boardName[15];
        getWSBBoardName(boardName, sizeof(boardName));
        char* taskName = pcTaskGetName(NULL);
        DEBUG_PRINT("Expected: [%s], got wsb: %s, deleting %s\n", validNames, boardName, taskName);
        vTaskDelete(NULL);
        return true;
    }
    return false;
}