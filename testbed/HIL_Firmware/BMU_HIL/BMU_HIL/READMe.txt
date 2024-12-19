The BMU HIL code has preliminary setup for:

GPIO pins, PWM outputs, and RMT input for measuring duty cycles.
Code to translate CAN messages into GPIO outputs.
FreeRTOS functionality.

I have only tested GPIO and FreeRTOS thread functionality. Those both work.

All other features have to be tested and debugged.