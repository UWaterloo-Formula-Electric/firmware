#ifndef ERRORHANDLER_H

#define ERRORHANDLER_H
#include "main.h"

#if IS_BOARD_NUCLEO_F7
#ifndef Error_Handler
#define Error_Handler() _Error_Handler(__FILE__, __LINE__)
#endif
#endif

// Runtime assert
#define c_assert(e) ((e) ? (0) : log_assert_violation(__FILE__,__LINE__,#e))
int log_assert_violation(char *file, int line, char *condition);

#endif /* end of include guard: ERRORHANDLER_H */
