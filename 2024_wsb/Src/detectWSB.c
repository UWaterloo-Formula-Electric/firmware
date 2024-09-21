#include "detectWSB.h"

#include "main.h"
#include "stm32f4xx_hal.h"
#include <string.h>

/**
 * @brief Detects the WSB type based on the front/back & left/right DIP switches
 */
WSBType detectWSB() {
    GPIO_PinState front = HAL_GPIO_ReadPin(FrontRear_DipSW_GPIO_Port, FrontRear_DipSW_Pin);
    GPIO_PinState left = HAL_GPIO_ReadPin(LeftRight_DipSW_GPIO_Port, LeftRight_DipSW_Pin);

    if (front == GPIO_PIN_SET) {
        return left == GPIO_PIN_SET ? WSBFL : WSBFR;
    } else {
        return left == GPIO_PIN_SET ? WSBRL : WSBRR;
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
