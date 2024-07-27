BUILD_TARGET = wsb
BOARD_ARCHITECTURE = F0

COMMON_LIB_SRC = userCan.c uwfe_debug.c state_machine.c FreeRTOS_CLI.c freertos_openocd_hack.c watchdog.c generalErrorHandler.c canReceiveCommon.c
COMMON_F0_LIB_SRC = userCanF0.c


F7_INC_DIR := 
F7_SRC_DIR := 
F7_SRC := 


CUBE_F0_MAKEFILE_PATH= $(BUILD_TARGET)/Cube-F0-Src/2018_WSB/


wsb: wsbfl wsbfr wsbrr wsbrl

BOARD_NAME = wsbfl
BOARD_NAME_UPPER = WSBFL
include common/tail.mk

BOARD_NAME = wsbfr
BOARD_NAME_UPPER = WSBFR
include common/tail.mk

BOARD_NAME = wsbrr
BOARD_NAME_UPPER = WSBRR
include common/tail.mk

BOARD_NAME = wsbrl
BOARD_NAME_UPPER = WSBRL
include common/tail.mk
