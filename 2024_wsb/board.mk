BUILD_TARGET = 2024_wsb
BOARD_ARCHITECTURE = F4

COMMON_LIB_SRC = userCan.c uwfe_debug.c state_machine.c FreeRTOS_CLI.c freertos_openocd_hack.c watchdog.c generalErrorHandler.c canReceiveCommon.c
COMMON_F4_LIB_SRC = userCanF4.c

F4_INC_DIR :=  # handled by tail.mk in common
F4_SRC_DIR :=  # handled by tail.mk in common
F4_SRC :=  	   # handled by tail.mk in common

CUBE_F4_MAKEFILE_PATH= $(BUILD_TARGET)/Cube-F4-Src/

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
