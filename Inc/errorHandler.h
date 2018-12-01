#ifndef ERRORHANDLER_H

#define ERRORHANDLER_H
#include "main.h"

#ifndef Error_Handler
#define Error_Handler() _Error_Handler(__FILE__, __LINE__)
#endif

void _handleError(char *file, int line);

#endif /* end of include guard: ERRORHANDLER_H */
