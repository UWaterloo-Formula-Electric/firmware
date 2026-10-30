#ifndef CHARGER_H
#define CHARGER_H

#include "chargerRegs.h"
#include <stdint.h>

typedef enum {
    CHARGER_OK, // charger IC is present and responding
    CHARGER_BUS_ERROR, //retried in 5 tries, at least 1 ms apart, and was unable to read
    CHARGER_WRONG_CHIP, // wrong chip ID read, mark the charger as unavailalbe, release CE
    CHARGER_INVALID_ARG // invalid pointer 
} bqReadStatus_t;

bqReadStatus_t chargerVerifyChip(uint8_t *data);

// two helper functions are used to decode the partand revision nubmer from the part info register
uint8_t chargerDecodeRevision(uint8_t data);

uint8_t chargerDecodePartNumber(uint8_t data);


#endif  // CHARGER_H