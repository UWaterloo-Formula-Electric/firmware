BUILD_TARGET = pdu
BOARD_NAME = pdu
BOARD_NAME_UPPER = PDU
BOARD_ARCHITECTURE = F7

COMMON_LIB_SRC = userCan.c uwfe_debug.c state_machine.c freertos_openocd_hack.c FreeRTOS_CLI.c generalErrorHandler.c watchdog.c canHeartbeat.c canReceiveCommon.c
COMMON_F7_LIB_SRC = userCanF7.c

F7_INC_DIR := 
F7_SRC_DIR := 
F7_SRC := 

CUBE_F7_MAKEFILE_PATH = $(BOARD_NAME)/Cube-F7-Src-respin/
CUBE_NUCLEO_MAKEFILE_PATH = $(BOARD_NAME)/Cube-Nucleo-Src/CanTest/

include common/tail.mk
