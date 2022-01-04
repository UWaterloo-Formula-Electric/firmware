BUILD_TARGET = vcu
BOARD_NAME = vcu_F7
BOARD_NAME_UPPER = VCU_F7
BOARD_ARCHITECTURE = F7

COMMON_LIB_SRC = userCan.c debug.c state_machine.c CRC_CALC.c FreeRTOS_CLI.c freertos_openocd_hack.c watchdog.c canHeartbeat.c generalErrorHandler.c 
COMMON_F7_LIB_SRC = userCanF7.c

F7_INC_DIR := 
F7_SRC_DIR := 
F7_SRC := 

CUBE_F7_MAKEFILE_PATH = $(BUILD_TARGET)/Cube-F7-Src-respin/
CUBE_NUCLEO_MAKEFILE_PATH = $(BUILD_TARGET)/Cube-Nucleo-Src/CanTest/

include common/tail.mk

