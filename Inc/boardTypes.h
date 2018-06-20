#ifndef __BOARD_TYPES_H
#define __BOARD_TYPES_H

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

#endif // __BOARD_TYPES_H
