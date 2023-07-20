#ifndef DIGITAL_POT_H
#define DIGITAL_POT_H
 
//check data sheet for AD5260BRUZ200 to retreive values
#define WIPER_RESISTANCE_OHM 60
#define MAX_DIGITAL_VALUE 256
#define NOMINAL_RESISTANCE_OHM 200000
#define LSB_OHM ((1*NOMINAL_RESISTANCE_OHM)/MAX_DIGITAL_VALUE+WIPER_RESISTANCE_OHM) //841 OHM
#define POT_MAX NOMINAL_RESISTANCE_OHM-LSB_OHM+WIPER_RESISTANCE_OHM //199,219 OHM

void pot_task (void * pvParameters);
int setPotResistance (uint32_t resistance);

#endif