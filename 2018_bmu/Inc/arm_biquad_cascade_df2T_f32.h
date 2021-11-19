#ifndef ARM_BIQUAD_CASCADE_DF2T_F32_H

#define ARM_BIQUAD_CASCADE_DF2T_F32_H

#include "bsp.h"

#define LOW_OPTIMIZATION_ENTER __attribute__(( optimize("-O1") ))
#define LOW_OPTIMIZATION_EXIT

#define ARM_MATH_CM7

typedef float float32_t;

  /**
   * @brief Instance structure for the floating-point transposed direct form II Biquad cascade filter.
   */
  typedef struct
  {
    uint8_t numStages;         /**< number of 2nd order stages in the filter.  Overall order is 2*numStages. */
    float32_t *pState;         /**< points to the array of state coefficients.  The array is of length 2*numStages. */
    float32_t *pCoeffs;        /**< points to the array of coefficients.  The array is of length 5*numStages. */
  } arm_biquad_cascade_df2T_instance_f32;

void arm_biquad_cascade_df2T_f32(
const arm_biquad_cascade_df2T_instance_f32 * S,
float32_t * pSrc,
float32_t * pDst,
uint32_t blockSize);

#endif /* end of include guard: ARM_BIQUAD_CASCADE_DF2T_F32_H */
