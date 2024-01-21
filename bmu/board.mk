BUILD_TARGET = bmu
BOARD_NAME = bmu
BOARD_NAME_UPPER = BMU
BOARD_ARCHITECTURE = F7

COMMON_LIB_SRC := userCan.c debug.c state_machine.c FreeRTOS_CLI.c freertos_openocd_hack.c watchdog.c canHeartbeat.c generalErrorHandler.c canReceiveCommon.c ade7913_common.c
COMMON_F7_LIB_SRC := userCanF7.c

F7_INC_DIR := $(BOARD_NAME)/Inc/F7_Inc
F7_SRC_DIR := $(BOARD_NAME)/Src/F7_Src
F7_SRC := ltc6804.c ltc6812.c ltc_chip.c ltc_common.c imdDriver.c

CUBE_F7_MAKEFILE_PATH := $(BOARD_NAME)/Cube-F7-Src-respin/
CUBE_NUCLEO_MAKEFILE_PATH := $(BOARD_NAME)/Cube-Nucleo-Src/CanTest/
DOCS_MAKEFILE_PATH := $(BOARD_NAME)/Docs/sphinx/

include common/tail.mk
