BUILD_TARGET = HIL
BOARD_NAME = HIL
BOARD_NAME_UPPER = HIL
BOARD_ARCHITECTURE = F7
MCU_DEFINE = STM32F769xx

# HIL is a bench-only CAN test/simulation board that never ships in the car.
# It is still the HIL node in common/Data/2024CAR.dbc, so its CAN and DTC code
# is generated into Gen/HIL/ the same as every vehicle board (see README.md).

COMMON_LIB_SRC := userCan.c debug.c FreeRTOS_CLI.c freertos_openocd_hack.c newlibHack.c generalErrorHandler.c
COMMON_F7_LIB_SRC := userCanF7.c

CUBE_F7_MAKEFILE_PATH := $(BOARD_NAME)/Cube-F7-Src-respin/

include common/tail.mk
