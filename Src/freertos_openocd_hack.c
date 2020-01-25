/*
 * This file should be include in the COMMON_LIB_SRC variable in the project
 * Makefile if you want gdb to be aware of FreeRTOS threads. Then using make
 * gdb will allow you to do things like info threads to see all FreeRTOS tasks
 * See: http://www.openocd.org/doc/html/GDB-and-OpenOCD.html
 * https://electronics.stackexchange.com/questions/112931/using-rtos-support-in-openocd
 * https://github.com/arduino/OpenOCD/blob/master/contrib/rtos-helpers/FreeRTOS-openocd.c
 */

/*
 * Since at least FreeRTOS V7.5.3 uxTopUsedPriority is no longer
 * present in the kernel, so it has to be supplied by other means for
 * OpenOCD's threads awareness.
 *
 * Add this file to your project, and, if you're using --gc-sections,
 * ``--undefined=uxTopUsedPriority'' (or
 * ``-Wl,--undefined=uxTopUsedPriority'' when using gcc for final
 * linking) to your LDFLAGS; same with all the other symbols you need.
 */

#include "FreeRTOS.h"

#ifdef __GNUC__
#define USED __attribute__((used))
#else
#define USED
#endif

const int USED uxTopUsedPriority = configMAX_PRIORITIES - 1;
