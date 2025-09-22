CUSTOM_COMMANDS = all clean autogen init lint load connect gdb test
BOARDS = bmu pdu dcu vcu wsb wsbfl wsbfr wsbrr wsbrl beaglebone

.PHONY: $(CUSTOM_COMMANDS) $(BOARDS) 

all: bmu dcu pdu vcu wsb
 
beaglebone:;
	make -C beaglebone/os/

dashboard:;
	make -C beaglebone/app/wfe/dashboard/

include cellTester/board.mk
include bmu/board.mk
# include dcu/board.mk
include pdu/board.mk
include vcu/board.mk
#include wsb/board.mk
include 2024_wsb/board.mk # change the name to wsb once firmware bringup is complete
