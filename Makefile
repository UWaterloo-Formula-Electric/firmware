CUSTOM_COMMANDS = all clean autogen init lint
BOARDS = bmu pdu dcu vcu wsb
.PHONY: $(CUSTOM_COMMANDS) $(BOARDS) 

all: bmu dcu pdu vcu

include bmu/board.mk
include dcu/board.mk
include pdu/board.mk
include vcu/board.mk
