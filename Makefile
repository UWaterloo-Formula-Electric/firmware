CUSTOM_COMMANDS = all clean autogen
BOARDS = bmu pdu dcu vcu wsb
.PHONY: $(CUSTOM_COMMANDS) $(BOARDS) 
BIN_DIR = Bin

all: bmu

include bmu/board.mk

