#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "debug.h"
#include "uartRXTask.h"
#include "fetControl.h"

#define INPUT_BUFFER_SIZE (100)
static char rxString[INPUT_BUFFER_SIZE];
uint32_t rxIndex = 0;
static float current_target = 0.0f;

BaseType_t processInput(char* inputString);

void uartRXTask(void const* argument) {
    while (1) {
        char rxBuffer;
        if (xQueueReceive(uartRxQueue, &rxBuffer, portMAX_DELAY) != pdTRUE) {
            ERROR_PRINT("Error Receiving from UART Rx Queue\n");
            handleError();
        }

        if (rxBuffer == '\n') {
            /* A newline character was received, so the input command string is
            complete and can be processed. */
            processInput(rxString);

            /* All the strings generated by the input command have been sent.
            Processing of the command is complete.  Clear the input string ready
            to receive the next command. */
            rxIndex = 0;
            memset(rxString, 0x00, INPUT_BUFFER_SIZE);
        } else {
            /* The if() clause performs the processing after a newline character
            is received.  This else clause performs the processing if any other
            character is received. */

            if (rxBuffer == '\r') {
                /* Ignore carriage returns. */
            } else if (rxBuffer == '\b') {
                /* Backspace was pressed.  Erase the last character in the input
                buffer - if there are any. */
                if (rxIndex > 0) {
                    rxIndex--;
                    rxString[rxIndex] = '\0';
                }
            } else {
                /* A character was entered.  It was not a new line, backspace
                or carriage return, so it is accepted as part of the input and
                placed into the input buffer.  When a \n is entered the complete
                string will be passed to the command interpreter. */
                if (rxIndex < INPUT_BUFFER_SIZE) {
                    rxString[rxIndex] = rxBuffer;
                    rxIndex++;
                } else {
                    ERROR_PRINT("Rx string buffer overflow\n");
                }
            }
        }
    }
}

BaseType_t processInput(char* inputString) {
    current_target = (float)atof(inputString);
    DEBUG_PRINT("desired current set to: %.4f\r\n", current_target);
    
    // Reset PWM
    set_PWM_Duty_Cycle(&FET_TIM_HANDLE, 0.0f);
    return pdFALSE;
}

float getCurrentTarget(void)
{
    return current_target;
}