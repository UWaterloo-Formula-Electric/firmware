CUSTOM_COMMANDS = all clean autogen init lint
BOARDS = bmu pdu dcu vcu wsb beaglebone
.PHONY: $(CUSTOM_COMMANDS) $(BOARDS) 

all: bmu dcu pdu vcu

beaglebone:;
	make -C beaglebone/os/


include bmu/board.mk
include dcu/board.mk
include pdu/board.mk
include vcu/board.mk
