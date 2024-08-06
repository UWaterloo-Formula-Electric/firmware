BUILD_TARGET = tcu
BOARD_NAME = tcu
BOARD_NAME_UPPER = TCU
BOARD_ARCHITECTURE = F4

COMMON_LIB_SRC = userCan.c uwfe_debug.c watchdog.c state_machine.c freertos_openocd_hack.c FreeRTOS_CLI.c generalErrorHandler.c canReceiveCommon.c
COMMON_F4_LIB_SRC = userCanF4.c

F4_INC_DIR := 
F4_SRC_DIR := 
F4_SRC := 

CUBE_F4_MAKEFILE_PATH= $(BUILD_TARGET)/Cube-F4-Src/

include common/tail.mk