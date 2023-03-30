#ifndef GPIO_PORTS_H
#define GPIO_PORTS_H
#include "gpio.h"

#define BUT1_Pin (GPIO_PIN_13)
#define BUT1_GPIO_Port (GPIOC)
#define BUT2_Pin (GPIO_PIN_14)
#define BUT2_GPIO_Port (GPIOC)
#define BUT3_Pin (GPIO_PIN_15)
#define BUT3_GPIO_Port (GPIOC)

#define LED_R_Pin (GPIO_PIN_0)
#define LED_R_GPIO_Port (GPIOC)
#define LED_Y_Pin (GPIO_PIN_1)
#define LED_Y_GPIO_Port (GPIOC)
#define LED_B_Pin (GPIO_PIN_2)
#define LED_B_GPIO_Port (GPIOC)

#define Throttle_A_Pin (GPIO_PIN_4)
#define Throttle_A_GPIO_Port (GPIOA)
#define Throttle_B_Pin (GPIO_PIN_5)
#define Throttle_B_GPIO_Port (GPIOA)

#define Brake_Pos_Pin (GPIO_PIN_6)
#define Brake_Pos_GPIO_Port (GPIOA)

#define Steering_Pin (GPIO_PIN_7)
#define Steering_GPIO_Port (GPIOA)

#define Break_Pres_Pin (GPIO_PIN_0)
#define Break_Pres_GPIO_Port (GPIOB)

#define PP_BB_EN_Pin (GPIO_PIN_8)
#define PP_BB_EN_GPIO_Port (GPIOD)
#define PP_5V0_EN_Pin (GPIO_PIN_9)
#define PP_5V0_EN_GPIO_Port (GPIOD)

#endif