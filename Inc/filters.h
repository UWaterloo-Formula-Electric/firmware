#ifndef FILTERS_H

#define FILTERS_H

#include "bsp.h"

void filtersInit();

void lowPassFilter(float *samples, int numSamples, float *output);

#endif /* end of include guard: FILTERS_H */
