#ifndef __DETECTWSB_H__
#define __DETECTWSB_H__

#include "boardTypes.h"
#include <stdbool.h>
#include <stddef.h>
// The wsbs are configured with 2 dip switches that can be used to select if they are fl, fr, rl, or rr
typedef enum {
    INVALID_WSB = 0,
    WSBFL = 1,
    WSBFR = 2,
    WSBRL = 4,
    WSBRR = 8
} WSBType_t;

WSBType_t detectWSB();
bool getWSBBoardName(char* boardName, size_t size);
bool deleteWSBTask(uint8_t validWSBs);
#endif  // __DETECTWSB_H__