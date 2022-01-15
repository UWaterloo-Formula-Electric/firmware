BUILD_TARGET = wsb
BOARD_NAME = wsb
BOARD_NAME_UPPER = WSB
BOARD_ARCHITECTURE = F0

COMMON_LIB_SRC = userCan.c debug.c state_machine.c CRC_CALC.c FreeRTOS_CLI.c freertos_openocd_hack.c watchdog.c generalErrorHandler.c
COMMON_F0_LIB_SRC = userCanF0.c


F7_INC_DIR := 
F7_SRC_DIR := 
F7_SRC := 


CUBE_F0_MAKEFILE_PATH= $(BOARD_NAME)/Cube-F0-Src/2018_WSB/


include common/tail.mk

