#include "bsp.h"
#include "debug.h"

void DTC_Fatal_Callback(BoardIDs board)
{
    ERROR_PRINT("Fatal Callback received!\n");
}
