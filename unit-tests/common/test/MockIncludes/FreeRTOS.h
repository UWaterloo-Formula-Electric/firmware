#ifndef FAKE_FREERTOS_H
#define FAKE_FREERTOS_H
#include "fake_hal_defs.h"

#define pdMS_TO_TICKS(ms) (ms * 1000)
#define __weak  __attribute__((weak)) 



#endif
