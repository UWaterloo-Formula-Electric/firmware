#ifndef CHARGER_H
#define CHARGER_H

#include "chargerRegs.h"
#include <stdint.h>

typedef enum {
    CHARGER_OK, // charger IC is present and responding
    CHARGER_BUS_ERROR, //retried in 5 tries, at least 1 ms apart, and was unable to read
    CHARGER_WRONG_CHIP // wrong chip ID read, mark the charger as unavailalbe, release CE
} bqReadStatus_t;

bqReadStatus_t chargerVerifyChip(uint8_t reg, uint8_t *data);

#endif  // CHARGER_H