#ifndef __BOARD_TYPES_H
#define __BOARD_TYPES_H

#include "stdint.h"

// Possible Board Types
typedef enum BoardTypes_t {
    INVALID_BOARD_TYPE,
    NUCLEO_F7,
    F7,
    NUCLEO_F0,
    F0
} BoardTypes_t;

extern const BoardTypes_t boardType;

#define IS_BOARD_F0_FAMILY \
    (BOARD_TYPE_F0 || BOARD_TYPE_NUCLEO_F0)

#define IS_BOARD_F7_FAMILY \
    (BOARD_TYPE_F7 || BOARD_TYPE_NUCLEO_F7)

#define IS_BOARD_F0 \
    (BOARD_TYPE_F0)
#define IS_BOARD_F7 \
    (BOARD_TYPE_F7)
#define IS_BOARD_NUCLEO_F0 \
    (BOARD_TYPE_NUCLEO_F0)
#define IS_BOARD_NUCLEO_F7 \
    (BOARD_TYPE_NUCLEO_F7)

// CAN IDs of the Boards
#define ID_BMU            1
#define ID_VCU_F7         2
#define ID_PDU            3
#define ID_ChargeCart     4
#define ID_MCLeft         5
#define ID_MCRight        6
#define ID_DCU            7
#define ID_WSBFL          8
#define ID_WSBFR          9
#define ID_WSBRL         10
#define ID_WSBRR         11
#define ID_VCU_BeagleBone 12
#define ID_CELLTESTER    13
#define ID_Charger       14
#define ID_Computer      15

#define BOARD_IS_WSB(ID) \
    ((ID) == ID_WSBFL || \
     (ID) == ID_WSBFR || \
     (ID) == ID_WSBRL || \
     (ID) == ID_WSBRR)

typedef uint32_t BoardIDs;

#endif // __BOARD_TYPES_H
