BINARY_BASE_NAME=BMU
BOARD_NAME=BMU

COMMON_LIB_SRC = userCan.c debug.c state_machine.c CRC_CALC.c FreeRTOS_CLI.c freertos_openocd_hack.c watchdog.c canHeartbeat.c
COMMON_F7_LIB_SRC = userCanF7.c

F7_INC_DIR = Inc/F7_Inc
F7_SRC_DIR = Src/F7_Src
F7_SRC = ltc6811.c ade7912.c imdDriver.c

# one of NUCLEO_F7, F7
BOARD_TYPE ?= F7

CUBE_F7_MAKEFILE_PATH= Cube-F7-Src/
CUBE_NUCLEO_MAKEFILE_PATH = Cube-Nucleo-Src/CanTest/

include common-all/tail.mk
