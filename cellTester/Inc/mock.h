#ifndef __MOCK_H__
#define __MOCK_H__
#include "task.h"
// These are stubs to be replaced by actual code 
double get_cell_voltage() { return 0; }
double get_cell_temperature() { return 0; }
double get_cell_current() { return 0; }

void temperatureTask(void *args) {while(1) {vTaskDelay(10000);}}
#endif  // __MOCK_H__