BINARY_BASE_NAME=PDU
BOARD_NAME=PDU

COMMON_LIB_SRC = userCan.c debug.c state_machine.c CRC_CALC.c freertos_openocd_hack.c FreeRTOS_CLI.c generalErrorHandler.c watchdog.c canHeartbeat.c

COMMON_F7_LIB_SRC = userCanF7.c

# one of NUCLEO_F7, F7
BOARD_TYPE ?= F7
ifeq ($(BOARD_TYPE), F7)
BOARD_VERSION ?= 2
else
BOARD_VERSION ?= 1
endif

CUBE_F7_VERSION_1_MAKEFILE_PATH= Cube-F7-Src/
CUBE_F7_VERSION_2_MAKEFILE_PATH= Cube-F7-Src-respin/

CUBE_NUCLEO_MAKEFILE_PATH = Cube-Nucleo-Src/CanTest/

include ../common-all/tail.mk
