#ifndef __DETECTWSB_H__
#define __DETECTWSB_H__

#include "boardTypes.h"
#include <stdbool.h>
#include <stddef.h>
// The wsbs are configured with 2 dip switches that can be used to select if they are fl, fr, rl, or rr
typedef enum {
    INVALID_WSB = 0,
    WSBFL = ID_WSBFL,
    WSBFR = ID_WSBFR,
    WSBRL = ID_WSBRL,
    WSBRR = ID_WSBRR
} WSBType;

WSBType detectWSB();
bool getWSBBoardName(char* boardName, size_t size);

#endif  // __DETECTWSB_H__