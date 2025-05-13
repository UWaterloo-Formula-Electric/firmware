BUILD_TARGET = pdu_hil
BOARD_NAME = pdu_hil
BOARD_NAME_UPPER = PDU_HIL
BOARD_ARCHITECTURE = F7

COMMON_LIB_SRC := userCan_HIL.c debug_HIL.c state_machine.c FreeRTOS_CLI.c freertos_openocd_hack.c
COMMON_F7_LIB_SRC := userCanF7.c

# first Inc/Src
F7_INC_DIR := 
F7_SRC_DIR := 
F7_SRC := 

CUBE_F7_MAKEFILE_PATH := testbed/HIL_Firmware/$(BOARD_NAME)/Cube-F7-Src-respin/
include common/tail_HIL.mk