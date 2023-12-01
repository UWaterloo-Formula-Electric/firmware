BUILD_TARGET = dcu
BOARD_NAME = dcu
BOARD_NAME_UPPER = DCU
BOARD_ARCHITECTURE = F0

COMMON_LIB_SRC := userCan.c debug.c state_machine.c FreeRTOS_CLI.c freertos_openocd_hack.c watchdog.c canHeartbeat.c generalErrorHandler.c canReceiveCommon.c
COMMON_F0_LIB_SRC := userCanF0.c

CUBE_F0_MAKEFILE_PATH := $(BOARD_NAME)/Cube-F0-Src/DCU/
CUBE_NUCLEO_F0_MAKEFILE_PATH := $(BOARD_NAME)/Cube-Nucleo-Src/DCU/

DOCS_MAKEFILE_PATH := $(BOARD_NAME)/Docs/sphinx/

include common/tail.mk
