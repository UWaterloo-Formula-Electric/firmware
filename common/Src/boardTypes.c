#include "boardTypes.h"

#ifdef BOARD_TYPE_F7
const BoardTypes_t boardType = F7;
#elif defined(BOARD_TYPE_F0)
const BoardTypes_t boardType = F0;
#elif defined(BOARD_TYPE_F4)
const BoardTypes_t boardType = F4;
#elif defined(BOART_TYPE_NUCLEO_F7)
const BoardTypes_t boardType = NUCLEO_F7;
#elif defined(BOART_TYPE_NUCLEO_F0)
const BoardTypes_t boardType = NUCLEO_F0;
#elif defined(BOART_TYPE_NUCLEO_F4)
const BoardTypes_t boardType = NUCLEO_F4;
#else
#error Invalid board type
#endif
