CUSTOM_COMMANDS = all clean autogen init lint
BOARDS = bmu pdu dcu vcu wsb
.PHONY: $(CUSTOM_COMMANDS) $(BOARDS) 

LINT_DIR = Lint
LINT_TARGETS := common bmu pdu dcu vcu wsb
LINT_TARGET ?= none
RUN_LINTER = cppcheck --addon=misra.py --enable=all  -I $(LINT_TARGET)/Inc --output-file=$(LINT_DIR)/$(LINT_TARGET)-lint.log $(LINT_TARGET)/Src

all: bmu dcu pdu vcu

lint:
	@mkdir -p $(LINT_DIR)
ifneq ($(filter $(LINT_TARGET), $(LINT_TARGETS)),)
	@echo "Running linter on: $(LINT_TARGET)"
	$(RUN_LINTER)
else
	@echo "Running linter on: $(LINT_TARGETS)"
	$(foreach LINT_TARGET, $(LINT_TARGETS), $(RUN_LINTER);)
endif

include bmu/board.mk
include dcu/board.mk
include pdu/board.mk
include vcu/board.mk
