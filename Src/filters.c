#include "filters.h"
#include "arm_biquad_cascade_df2T_f32.h"
/*#include "iir_fs200_pass10_stop60_apass1_astop80.h"*/
#include "iir_fs200_pass80_stop90_apass1_astop80.h"

arm_biquad_cascade_df2T_instance_f32 lowPassFilterInstance;
float32_t lowPassFilter_coeffs[5*NUM_SECTIONS] = {0};
float32_t lowPassFilter_state[4*NUM_SECTIONS] = {0};

void filtersInit()
{
   int k=0;
   for (int i=0; i<NUM_SECTIONS; i++)
   {
      lowPassFilter_coeffs[k++] = b[i][0];
      lowPassFilter_coeffs[k++] = b[i][1];
      lowPassFilter_coeffs[k++] = b[i][2];
      lowPassFilter_coeffs[k++] = -a[i][1];
      lowPassFilter_coeffs[k++] = -a[i][2];
   }
   lowPassFilterInstance.numStages = NUM_SECTIONS;
   lowPassFilterInstance.pState = lowPassFilter_state;
   lowPassFilterInstance.pCoeffs = lowPassFilter_coeffs;

}

void lowPassFilter(float *samples, int numSamples, float *output)
{
   arm_biquad_cascade_df2T_f32(&lowPassFilterInstance, samples, output, numSamples);
}
