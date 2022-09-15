CUSTOM_COMMANDS = all clean autogen init lint load connect gdb set_debug load-debug
BOARDS = bmu pdu dcu vcu wsb wsbfl wsbfr wsbrr wsbrl beaglebone
# DEBUG = 0
# export DEBUG
.PHONY: $(CUSTOM_COMMANDS) $(BOARDS) 

all: bmu dcu pdu vcu wsb
 
beaglebone:;
	make -C beaglebone/os/


include bmu/board.mk
include dcu/board.mk
include pdu/board.mk
include vcu/board.mk
include wsb/board.mk
