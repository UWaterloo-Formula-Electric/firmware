BINARY_BASE_NAME=VCU
BOARD_NAME=VCU_F7

COMMON_LIB_SRC = userCan.c debug.c

BOARD_TYPE=F7 # one of NUCLEO-F7, F7

include common-all/tail-f7.mk
