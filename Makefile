CUSTOM_COMMANDS = all clean autogen init load
BOARDS = bmu pdu dcu vcu wsb wsbfl wsbfr wsbrr wsbrl
.PHONY: $(CUSTOM_COMMANDS) $(BOARDS) 

all: bmu dcu pdu vcu

include bmu/board.mk
include dcu/board.mk
include pdu/board.mk
include vcu/board.mk
include wsb/board.mk
