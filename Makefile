BINARY_BASE_NAME=VCU
BOARD_NAME=VCU_F7

COMMON_LIB_SRC = userCan.c debug.c state_machine.c

# one of NUCLEO_F7, F7
BOARD_TYPE ?= NUCLEO_F7

CUBE_F7_MAKEFILE_PATH= Cube-F7-Src/2018_pdu/
CUBE_NUCLEO_MAKEFILE_PATH = Cube-Nucleo-Src/CanTest/

include common-all/tail-f7.mk
