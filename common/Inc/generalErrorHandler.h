#ifndef GENERALERRORHANDLER_H

#define GENERALERRORHANDLER_H

#define handleError() (_handleError(__FILE__, __LINE__))
void _handleError(char *file, int line);

#endif /* end of include guard: GENERALERRORHANDLER_H */
