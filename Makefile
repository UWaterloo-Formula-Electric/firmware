CUSTOM_COMMANDS = all clean autogen init lint load connect gdb test vcu_hil
BOARDS = bmu pdu dcu vcu wsb wsbfl wsbfr wsbrr wsbrl beaglebone vcu_hil

.PHONY: $(CUSTOM_COMMANDS) $(BOARDS) 

all: bmu dcu pdu vcu wsb cellTester vcu_hil
 
beaglebone:;
	make -C beaglebone/os/

dashboard:;
	make -C beaglebone/app/wfe/dashboard/

include cellTester/board.mk
include bmu/board.mk
include dcu/board.mk
include pdu/board.mk
include vcu/board.mk
include wsb/board.mk

include testbed/HIL_Firmware/vcu_hil/board.mk