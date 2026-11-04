
#include "linMock.h"
#include "linDriver.h"
#include "debug.h"
#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "stm32f7xx_hal.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

static LIN_MockResponse_t linMockResponse;
static uint8_t rxBuffer[LIN_RX_BUFFER_SIZE] __attribute__((aligned(32)));

void buildMockResponse(
    uint8_t id,
    uint8_t *data,
    uint8_t dataLength) {
    linMockResponse.id = id;
    linMockResponse.pid = calcProtectedID(linMockResponse.id);
    linMockResponse.length = dataLength + 1U;
    memcpy(linMockResponse.bytes, data, dataLength);

    linMockResponse.bytes[linMockResponse.length - 1U] = 
        calcChecksum(linMockResponse.id, linMockResponse.pid, data, dataLength);
}

HAL_StatusTypeDef mockLinPublish(
    uint8_t id,
    uint8_t *data,
    uint8_t dataLength
) {
    LIN_Frame_t frame = {.id = id, .isRxBuffer = false};

    HAL_StatusTypeDef status = buildFrame(&frame, data, dataLength);

    if (status != HAL_OK) {
        return status;
    }

    memcpy(rxBuffer, frame.buffer, frame.length);

    if (memcmp(rxBuffer, frame.buffer, frame.length) != 0) {
        ERROR_PRINT("Mismatch between transmitted frame and copy");
        return HAL_ERROR;
    }
    return HAL_OK;
}

HAL_StatusTypeDef mockLinRequest(
    uint8_t id,
    uint8_t *data,
    uint8_t dataLength) {
    LIN_Frame_t frame = {.id = id, .isRxBuffer = true};

    HAL_StatusTypeDef status = buildFrame(&frame, data, dataLength);

    if (status != HAL_OK) {
        return status;
    }

    memcpy(rxBuffer, frame.buffer, LIN_HEADER_SIZE);

    if (memcmp(rxBuffer, frame.buffer, LIN_HEADER_SIZE) != 0) {
        ERROR_PRINT("Mismatch between transmitted frame and copy");
        return HAL_ERROR;
    }
    uint8_t reqResponseLength = frame.length - LIN_HEADER_SIZE;
    // Set buffer to known state
    memset(rxBuffer + LIN_HEADER_SIZE, 0, reqResponseLength);
    // Emulate DMA reception truncation
    uint8_t bytesToCopy = linMockResponse.length < reqResponseLength ?
                            linMockResponse.length : reqResponseLength;

    memcpy(rxBuffer + LIN_HEADER_SIZE, linMockResponse.bytes, bytesToCopy);
    // Mirror actual implementation (checksum uses expected length)
    uint8_t expectedChecksum = calcChecksum(
        linMockResponse.id,
        linMockResponse.pid,
        rxBuffer + LIN_HEADER_SIZE,
        reqResponseLength - 1U
    );

    if (expectedChecksum != rxBuffer[frame.length - 1U]) {
        ERROR_PRINT("Received frame with invalid checksum");
        return HAL_ERROR;
    }
    memcpy(frame.buffer + LIN_HEADER_SIZE, rxBuffer + LIN_HEADER_SIZE,
        frame.length - LIN_HEADER_SIZE);
    return HAL_OK;
}

static BaseType_t cmdBuildMockResponse(
    char *writeBuffer,
    size_t writeBufferLength,
    const char *commandString) {
    BaseType_t paramLen;

    const char *idStr = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    if (idStr == NULL) {
        COMMAND_OUTPUT("Missing ID parameter\r\n");
        return pdFALSE;
    }
    uint8_t id = (uint8_t)strtoul(idStr, NULL, 0);

    if (id > LIN_MAX_FRAME_ID) {
        COMMAND_OUTPUT("ID must be in range 0x00 to 0x3D\r\n");
        return pdFALSE;
    }

    const char *dataLengthStr = FreeRTOS_CLIGetParameter(commandString, 2, &paramLen);
    if (dataLengthStr == NULL) {
        COMMAND_OUTPUT("Missing data length parameter\r\n");
        return pdFALSE;
    }
    uint8_t dataLength = (uint8_t)strtoul(dataLengthStr, NULL, 0);

    if (dataLength > LIN_MAX_DATA_SIZE) {
        COMMAND_OUTPUT("Data length cannot exceed 8 bytes\r\n");
        return pdFALSE;
    }

    const char *dataStr = FreeRTOS_CLIGetParameter(commandString, 3, &paramLen);
    if (dataStr == NULL) {
        COMMAND_OUTPUT("Missing data parameter\r\n");
        return pdFALSE;
    }
    
    uint8_t data[LIN_MAX_DATA_SIZE] = {0};
    const char *ptr = dataStr;

    for (uint8_t i = 0; i < dataLength; ++i) {
        if (ptr == NULL || *ptr == '\0') {
            COMMAND_OUTPUT("Too few bytes in data string\r\n");
            return pdFALSE;
        }

        data[i] = (uint8_t)strtoul(ptr, (char **)&ptr, 0);
        
        if (*ptr == ',') {
            ++ptr;
        }
    }

    buildMockResponse(id, data, dataLength);
    COMMAND_OUTPUT("Built mock LIN response having ID %d (%d bytes registered)\r\n", id, dataLength);
    return pdFALSE;
}

static CLI_Command_Definition_t linBuildMockResponseDefinition = {
    .pcCommand = "linbuildmockresponse",
    .pcHelpString = "linbuildmockresponse <id> <length> <data>:\r\nBuilds LIN mock response\r\n",
    .pxCommandInterpreter = cmdBuildMockResponse,
    .cExpectedNumberOfParameters = 3
};

static BaseType_t cmdMockLinPublish(
    char *writeBuffer,
    size_t writeBufferLength,
    const char *commandString) {
    BaseType_t paramLen;
    
    const char *idStr = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    if (idStr == NULL) {
        COMMAND_OUTPUT("Missing ID parameter\r\n");
        return pdFALSE;
    }
    uint8_t id = (uint8_t)strtoul(idStr, NULL, 0);

    if (id > LIN_MAX_FRAME_ID) {
        COMMAND_OUTPUT("ID must be in range 0x00 to 0x3D\r\n");
        return pdFALSE;
    }

    const char *dataLengthStr = FreeRTOS_CLIGetParameter(commandString, 2, &paramLen);
    if (dataLengthStr == NULL) {
        COMMAND_OUTPUT("Missing data length parameter\r\n");
        return pdFALSE;
    }
    uint8_t dataLength = (uint8_t)strtoul(dataLengthStr, NULL, 0);

    if (dataLength > LIN_MAX_DATA_SIZE) {
        COMMAND_OUTPUT("Data length cannot exceed 8 bytes\r\n");
        return pdFALSE;
    }

    const char *dataStr = FreeRTOS_CLIGetParameter(commandString, 3, &paramLen);
    if (dataStr == NULL && dataLength > 0) {
        COMMAND_OUTPUT("Missing data parameter\r\n");
        return pdFALSE;
    }

    uint8_t data[LIN_MAX_DATA_SIZE] = {0}; 
    const char *ptr = dataStr;
    for (uint8_t i = 0; i < dataLength; i++) {
        if (ptr == NULL || *ptr == '\0') {
            COMMAND_OUTPUT("Too few bytes in data string.\r\n");
            return pdFALSE;
        }

        data[i] = (uint8_t)strtoul(ptr, (char **)&ptr, 0);
        
        if (*ptr == ',') {
            ++ptr;
        }
    }

    HAL_StatusTypeDef status = mockLinPublish(id, data, dataLength);

    if (status == HAL_OK) {
        COMMAND_OUTPUT("Mock LIN publish succeeded for ID 0x%02X", id);
    }
    else {
        COMMAND_OUTPUT("Mock LIN publish failed with HAL status: %d", status);
    }
    return pdFALSE;
}

static CLI_Command_Definition_t linMockPublishDefinition = {
    .pcCommand = "linmockpublish",
    .pcHelpString = "linmockpublish <id> <length> <data>:\r\nMocks DMA reception validation for LIN frames transmitting data\r\n",
    .pxCommandInterpreter = cmdMockLinPublish,
    .cExpectedNumberOfParameters = 3
};

static BaseType_t cmdMockLinRequest(
    char *writeBuffer,
    size_t writeBufferLength,
    const char *commandString) {
    BaseType_t paramLen;
    
    const char *idStr = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    if (idStr == NULL) {
        COMMAND_OUTPUT("Missing ID parameter\r\n");
        return pdFALSE;
    }
    uint8_t id = (uint8_t)strtoul(idStr, NULL, 0);

    if (id > LIN_MAX_FRAME_ID) {
        COMMAND_OUTPUT("ID must be in range 0x00 to 0x3D\r\n");
        return pdFALSE;
    }

    const char *dataLengthStr = FreeRTOS_CLIGetParameter(commandString, 2, &paramLen);
    if (dataLengthStr == NULL) {
        COMMAND_OUTPUT("Missing data length parameter\r\n");
        return pdFALSE;
    }
    uint8_t dataLength = (uint8_t)strtoul(dataLengthStr, NULL, 0);

    if (dataLength > LIN_MAX_DATA_SIZE) {
        COMMAND_OUTPUT("Data length cannot exceed 8 bytes\r\n");
        return pdFALSE;
    }

    uint8_t rxBuffer[LIN_MAX_DATA_SIZE] = {0};

    HAL_StatusTypeDef status = mockLinRequest(id, rxBuffer, dataLength);

    if (status == HAL_OK) {
        char dataStr[64] = {0};
        uint8_t dataOffset = 0;

        for (uint8_t i = 0; i < dataLength; i++) {
            dataOffset += snprintf(dataStr + dataOffset, sizeof(dataStr) - dataOffset, "0x%02X ", rxBuffer[i]);
        }

        COMMAND_OUTPUT("Mock LIN request succeeded for ID 0x%02X. Data: %s\r\n", id, dataStr);
    } else {
        COMMAND_OUTPUT("Mock LIN request failed with HAL status: %d\r\n", status);
    }

    return pdFALSE;
}

static CLI_Command_Definition_t linMockRequestDefinition = {
    .pcCommand = "linmockrequest",
    .pcHelpString = "linmockrequest <id> <length>:\r\nMocks DMA reception validation for LIN frames requesting data\r\n",
    .pxCommandInterpreter = cmdMockLinRequest,
    .cExpectedNumberOfParameters = 2
};

static BaseType_t cmdLinPublish(
    char *writeBuffer,
    size_t writeBufferLength,
    const char *commandString) 
{
    BaseType_t paramLen;
    
    const char *idStr = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    if (idStr == NULL) {
        COMMAND_OUTPUT("Missing ID parameter\r\n");
        return pdFALSE;
    }
    uint8_t id = (uint8_t)strtoul(idStr, NULL, 0);

    if (id > LIN_MAX_FRAME_ID) {
        COMMAND_OUTPUT("ID must be in range 0x00 to 0x3D\r\n");
        return pdFALSE;
    }

    const char *dataLengthStr = FreeRTOS_CLIGetParameter(commandString, 2, &paramLen);
    if (dataLengthStr == NULL) {
        COMMAND_OUTPUT("Missing data length parameter\r\n");
        return pdFALSE;
    }
    uint8_t dataLength = (uint8_t)strtoul(dataLengthStr, NULL, 0);

    if (dataLength > LIN_MAX_DATA_SIZE) {
        COMMAND_OUTPUT("Data length cannot exceed 8 bytes\r\n");
        return pdFALSE;
    }

    const char *dataStr = FreeRTOS_CLIGetParameter(commandString, 3, &paramLen);
    if (dataStr == NULL && dataLength > 0) {
        COMMAND_OUTPUT("Missing data parameter\r\n");
        return pdFALSE;
    }

    uint8_t data[LIN_MAX_DATA_SIZE] = {0}; 
    const char *ptr = dataStr;
    for (uint8_t i = 0; i < dataLength; i++) {
        if (ptr == NULL || *ptr == '\0') {
            COMMAND_OUTPUT("Too few bytes in data string.\r\n");
            return pdFALSE;
        }

        data[i] = (uint8_t)strtoul(ptr, (char **)&ptr, 0);
        
        if (*ptr == ',') {
            ++ptr;
        }
    }

    HAL_StatusTypeDef status = linPublish(id, data, dataLength);

    if (status == HAL_OK) {
        COMMAND_OUTPUT("LIN publish succeeded for ID 0x%02X\r\n", id);
    } else {
        COMMAND_OUTPUT("LIN publish failed with HAL status: %d\r\n", status);
    }

    return pdFALSE;
}

static CLI_Command_Definition_t linPublishDefinition = {
    .pcCommand = "linpublish",
    .pcHelpString = "linpublish <id> <length> <data>:\r\nTransmit data via LIN bus\r\n",
    .pxCommandInterpreter = cmdLinPublish,
    .cExpectedNumberOfParameters = 3
};

static BaseType_t cmdLinRequest(
    char *writeBuffer,
    size_t writeBufferLength,
    const char *commandString) {
    BaseType_t paramLen;
    
    const char *idStr = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    if (idStr == NULL) {
        COMMAND_OUTPUT("Missing ID parameter\r\n");
        return pdFALSE;
    }
    uint8_t id = (uint8_t)strtoul(idStr, NULL, 0);

    if (id > LIN_MAX_FRAME_ID) {
        COMMAND_OUTPUT("ID must be in range 0x00 to 0x3D\r\n");
        return pdFALSE;
    }

    const char *dataLengthStr = FreeRTOS_CLIGetParameter(commandString, 2, &paramLen);
    if (dataLengthStr == NULL) {
        COMMAND_OUTPUT("Missing data length parameter\r\n");
        return pdFALSE;
    }
    uint8_t dataLength = (uint8_t)strtoul(dataLengthStr, NULL, 0);

    if (dataLength > LIN_MAX_DATA_SIZE) {
        COMMAND_OUTPUT("Data length cannot exceed 8 bytes\r\n");
        return pdFALSE;
    }

    uint8_t rxBuffer[LIN_MAX_DATA_SIZE] = {0};

    HAL_StatusTypeDef status = linRequest(id, rxBuffer, dataLength);

    if (status == HAL_OK) {
        char dataStr[64] = {0};
        uint8_t dataOffset = 0;

        for (uint8_t i = 0; i < dataLength; i++) {
            dataOffset += snprintf(dataStr + dataOffset, sizeof(dataStr) - dataOffset, "0x%02X ", rxBuffer[i]);
        }

        COMMAND_OUTPUT("LIN request succeeded for ID 0x%02X. Data: %s\r\n", id, dataStr);
    } else {
        COMMAND_OUTPUT("LIN request failed with HAL status: %d\r\n", status);
    }

    return pdFALSE;
}

static CLI_Command_Definition_t linRequestDefinition = {
    .pcCommand = "linrequest",
    .pcHelpString = "linrequest <id> <length>:\r\n Request data via LIN bus\r\n",
    .pxCommandInterpreter = cmdLinRequest,
    .cExpectedNumberOfParameters = 2
};

HAL_StatusTypeDef registerLinCommands(void) {
    if (FreeRTOS_CLIRegisterCommand(&linPublishDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&linRequestDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&linBuildMockResponseDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&linMockPublishDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&linMockRequestDefinition) != pdPASS) {
        return HAL_ERROR;
    }

    return HAL_OK;
}