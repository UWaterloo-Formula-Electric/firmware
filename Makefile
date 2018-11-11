BINARY_BASE_NAME=PDU
BOARD_NAME=PDU

COMMON_LIB_SRC = userCan.c debug.c state_machine.c CRC_CALC.c
COMMON_F7_LIB_SRC = userCanF7.c

# one of NUCLEO_F7, F7
BOARD_TYPE ?= NUCLEO_F7

CUBE_F7_MAKEFILE_PATH= Cube-F7-Src/2018_pdu/
CUBE_NUCLEO_MAKEFILE_PATH = Cube-Nucleo-Src/CanTest/

include common-all/tail.mk
