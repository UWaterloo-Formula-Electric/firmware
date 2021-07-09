CC=arm-none-eabi-gcc
HEX=arm-none-eabi-objcopy
SZ = arm-none-eabi-size

RM=rm -rf

SRC_DIR = Src
BIN_BASE_DIR = Bin
COMMON_LIB_DIR = common-all
COMMON_F7_LIB_DIR = common-all/f7
# only used to include some header files
COMMON_F0_LIB_DIR = common-all/f0
GITHOOKS_DIR = common-all/.githooks

MIDDLEWARE_DIR = Middlewares
FREERTOS_DIR = $(MIDDLEWARE_DIR)/Third_Party/FreeRTOS

GEN_DIR = Gen
GEN_INC_DIR = $(GEN_DIR)/Inc
GEN_SRC_DIR = $(GEN_DIR)/Src
COMMON_LIB_DATA_DIR = $(COMMON_LIB_DIR)/Data
DTC_CSV_FILE = $(COMMON_LIB_DATA_DIR)/dtc.csv
DBC_FILE = $(COMMON_LIB_DATA_DIR)/2018CAR.dbc

ELF_FILE = $(BINARY_BASE_NAME).elf
BIN_FILE = $(BINARY_BASE_NAME).bin
MAP_FILE = $(BINARY_BASE_NAME).map

# Set default version here, so not needed in makefile for single version boards
BOARD_VERSION ?= 1

#alias these for boards that only have one version
CUBE_F7_VERSION_1_MAKEFILE_PATH ?= $(CUBE_F7_MAKEFILE_PATH)
CUBE_NUCLEO_F7_VERSION_1_MAKEFILE_PATH ?= $(CUBE_NUCLEO_MAKEFILE_PATH)

ifeq ($(BOARD_TYPE), NUCLEO_F7)
   ifeq ($(BOARD_VERSION), 2)
      include $(CUBE_NUCLEO_F7_VERSION_2_MAKEFILE_PATH)/Cube-Lib.mk
   else ifeq ($(BOARD_VERSION), 1)
      include $(CUBE_NUCLEO_F7_VERSION_1_MAKEFILE_PATH)/Cube-Lib.mk
   else
      $(error "Unsupported Board version: $(BOARD_VERSION)")
   endif
else ifeq ($(BOARD_TYPE), F7)
   ifeq ($(BOARD_VERSION), 2)
   $(info "Board v2 path: $(CUBE_F7_VERSION_2_MAKEFILE_PATH)")
	include $(CUBE_F7_VERSION_2_MAKEFILE_PATH)/Cube-Lib.mk
   else ifeq ($(BOARD_VERSION), 1)
   $(info "Board v1 path: $(CUBE_F7_VERSION_1_MAKEFILE_PATH)")
	include $(CUBE_F7_VERSION_1_MAKEFILE_PATH)/Cube-Lib.mk
   else
      $(error "Unsupported Board version: $(BOARD_VERSION)")
   endif
else ifeq ($(BOARD_TYPE), NUCLEO_F0)
	include $(CUBE_NUCLEO_F0_MAKEFILE_PATH)/Cube-Lib.mk
else ifeq ($(BOARD_TYPE), F0)
	include $(CUBE_F0_MAKEFILE_PATH)/Cube-Lib.mk
else
	$(error "Unsupported Board type: $(BOARD_TYPE)")
endif

DEPDIR := .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

$(info $(C_INCLUDES))
INCLUDE_DIRS= $(COMMON_LIB_DIR)/Inc \
			  $(COMMON_F7_LIB_DIR)/Inc \
			  $(COMMON_F0_LIB_DIR)/Inc \
			  Inc \
			  $(F7_INC_DIR) \
			  $(GEN_INC_DIR)


INCLUDE_FLAGS := $(addprefix -I,$(INCLUDE_DIRS))
# add in driver dirs
INCLUDE_FLAGS += $(LIB_C_INCLUDES)
INCLUDE_FLAGS += $(LIB_AS_INCLUDES)


ifeq ($(BOARD_TYPE), $(filter $(BOARD_TYPE), NUCLEO_F7 F7))
DEFINES := "STM32F767xx"
else ifeq ($(BOARD_TYPE), $(filter $(BOARD_TYPE), NUCLEO_F0 F0))
DEFINES := "STM32F072xB"
else
	$(error "Unsupported Board type: $(BOARD_TYPE)")
endif
DEFINES += BOARD_NAME=$(BOARD_NAME) BOARD_ID=ID_$(BOARD_NAME) BOARD_TYPE_$(BOARD_TYPE)=1 "USE_HAL_DRIVER" BOARD_VERSION=$(BOARD_VERSION)
DEFINE_FLAGS := $(addprefix -D,$(DEFINES))

#ifeq '$(strip $(BOARD_TYPE))' '$(strip NUCLEO_F7)'
	#LINK_SCRIPT="STM32F767ZITx_FLASH.ld"
#else ifeq '$(strip $(BOARD_TYPE))' '$(strip F7)'
	#LINK_SCRIPT="STM32F767VITx_FLASH.ld"
#else
	#$(error Unsupported Board type)
#endif

#CPU = -mcpu=cortex-m7

#FPU = -mfpu=fpv5-d16

#FLOAT_ABI = -mfloat-abi=hard

#MCU = $(CPU) -mthumb $(FPU) $(FLOAT_ABI)

# -l is to link a library
#  -lc links libc.a, the c std lib
#  -lnosys links libnosys.a, has some stubbed system calls?
#  -lm links libm.a, the math library
#  -mthumb selects the thumb instruction set
#  -mcpu=cortex-m7 selects the cortex-m7 processor
#  -Wl--gc-sections passes --gc-sections to the linker, which means it only links used data and functions, and discards the rest
#  -T specifies the link script to use
#  -static On systems that support dynamic linking, this prevents linking with the shared libraries. On other systems, this option has no effect.
#  -Wl,--start-group -m -Wl,--end-group creates a group that is searched repeatedly for circular dependencies until no new undefined references are created
#  --cref, Output a cross reference table. If a linker map file is being generated, the cross reference table is printed to the map file
# -Wl,--defsym=malloc_getpagesize_P=0x1000, set the default page size of malloc to 0x1000, which means the heap increases in size by 4096 bytes at a time
#LINKER_FLAGS=-lc -lnosys -lm  -Wl,--gc-sections -T$(LINK_SCRIPT) -static  -Wl,--start-group -lm -Wl,--end-group -Wl,-cref "-Wl,-Map=$(MAP_FILE_PATH)" -Wl,--defsym=malloc_getpagesize_P=0x1000 $(MCU)

# inherit linker flags from hal driver makefile
LINKER_FLAGS =$(LIB_LDFLAGS)
LINKER_FLAGS += -Wl,-Map=$(MAP_FILE_PATH),--cref
LINKER_FLAGS += -u_printf_float -u_scanf_float
LINKER_FLAGS += -Wl,--undefined=uxTopUsedPriority
LINKER_FLAGS += -z muldefs
#DEBUG_FLAGS=-g -O2
#COMMON_FLAGS=-c $(DEBUG_FLAGS) -std=gnu99 -Wall $(MCU)
#ASSEMBLER_FLAGS=$(COMMON_FLAGS) -x assembler-with-cpp
ASSEMBLER_FLAGS = -x assembler-with-cpp $(LIB_ASFLAGS)

# -ffunction-sections and -fdata-sections, Place each function or data item into its own section in the output file
#  This is to allow linking only used functions and data
#COMPILER_FLAGS=$(COMMON_FLAGS) -ffunction-sections -fdata-sections $(DEFINE_FLAGS) -Werror $(DEPFLAGS)
COMPILER_FLAGS = $(LIB_CFLAGS)
COMPILER_FLAGS += $(DEFINE_FLAGS) $(DEPFLAGS) -Werror

POSTCOMPILE = @mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

SRC := $(wildcard $(SRC_DIR)/*.c) \
	   $(addprefix $(COMMON_LIB_DIR)/Src/, $(COMMON_LIB_SRC)) \
	   $(GEN_SRC_DIR)/$(BOARD_NAME)_can.c

ifeq ($(BOARD_TYPE), $(filter $(BOARD_TYPE), NUCLEO_F7 F7))
   SRC += $(addprefix $(COMMON_F7_LIB_DIR)/Src/, $(COMMON_F7_LIB_SRC))
else ifeq ($(BOARD_TYPE), $(filter $(BOARD_TYPE), NUCLEO_F0 F0))
   SRC += $(addprefix $(COMMON_F0_LIB_DIR)/Src/, $(COMMON_F0_LIB_SRC))
else
	$(error "Unsupported Board type: $(BOARD_TYPE)")
endif

ifeq ($(BOARD_TYPE), NUCLEO_F7)
   SRC += $(addprefix $(NUCLEO_F7_SRC_DIR)/, $(NUCLEO_F7_SRC))
else ifeq ($(BOARD_TYPE), F7)
   SRC += $(addprefix $(F7_SRC_DIR)/, $(F7_SRC))
else ifeq ($(BOARD_TYPE), NUCLEO_F0)
   SRC += $(addprefix $(NUCLEO_F0_SRC_DIR)/, $(NUCLEO_F0_SRC))
else ifeq ($(BOARD_TYPE), F0)
   SRC += $(addprefix $(F0_SRC_DIR)/, $(F0_SRC))
else
	$(error "Unsupported Board type: $(BOARD_TYPE)")
endif

ifeq ($(BOARD_NAME), BMU)
   SRC += $(GEN_SRC_DIR)/$(BOARD_NAME)_charger_can.c
endif

#
# add in driver sources
SRC += $(LIB_C_SOURCES)

SRCASM += $(LIB_ASM_SOURCES)

######################################
# Autogenerated code depedency files
######################################
SCRIPTS_DIR = $(COMMON_LIB_DIR)/Scripts
CAN_FILES_GEN_SCRIPT = $(SCRIPTS_DIR)/generateCANHeadder.py
DTC_FILES_GEN_SCRIPT = $(SCRIPTS_DIR)/generateDTC.py

GEN_DIR = Gen
GEN_FILES = $(GEN_SRC_DIR)/$(BOARD_NAME)_can.c \
	    $(GEN_INC_DIR)/$(BOARD_NAME)_can.h \
	    $(GEN_INC_DIR)/$(BOARD_NAME)_dtc.h
ifeq ($(BOARD_NAME), BMU)
   GEN_FILES += $(GEN_SRC_DIR)/$(BOARD_NAME)_charger_can.c
   GEN_FILES += $(GEN_INC_DIR)/$(BOARD_NAME)_charger_can.h
endif

###
#
# Release Build Settings
#
###

RELEASE_BIN_DIR = $(BIN_BASE_DIR)/Release
DEBUG_FLAGS=-g -O2

RELEASE_BIN_FILE = $(RELEASE_BIN_DIR)/$(BIN_FILE)
RELEASE_ELF_FILE = $(RELEASE_BIN_DIR)/$(ELF_FILE)
RELEASE_MAP_FILE = $(RELEASE_BIN_DIR)/$(MAP_FILE)
RELEASE_OBJS = $(SRC:%.c=$(RELEASE_BIN_DIR)/%.o) $(SRCASM:%.s=$(RELEASE_BIN_DIR)/%.o)

###
#
# Debug Build Settings
#
###

DEBUG_BIN_DIR = $(BIN_BASE_DIR)/Debug
DEBUG_FLAGS=-g -Og

DEBUG_BIN_FILE = $(DEBUG_BIN_DIR)/$(BIN_FILE)
DEBUG_ELF_FILE = $(DEBUG_BIN_DIR)/$(ELF_FILE)
DEBUG_MAP_FILE = $(DEBUG_BIN_DIR)/$(MAP_FILE)
DEBUG_OBJS = $(SRC:%.c=$(DEBUG_BIN_DIR)/%.o) $(SRCASM:%.s=$(DEBUG_BIN_DIR)/%.o)

.PHONY: clean test all debug release load connect connect-rtos gdb autogen docs

#
# Default build
#
all: release

#
# Initialization 
#
init:
	git config core.hooksPath $(GITHOOKS_DIR)
	git submodule init
	git submodule update

#
# Release target
#

release: BIN_DIR = $(RELEASE_BIN_DIR)
release: MAP_FILE_PATH = $(RELEASE_MAP_FILE)
release: BIN_FILE_PATH = $(RELEASE_BIN_FILE)
release: ELF_FILE_PATH = $(RELEASE_ELF_FILE)
release: $(RELEASE_BIN_FILE)

$(RELEASE_BIN_FILE): $(RELEASE_ELF_FILE)
	$(HEX) -O binary "$<" "$@"

$(RELEASE_ELF_FILE): $(RELEASE_OBJS)
	$(CC) $^ $(LINKER_FLAGS) -o $@
	$(SZ) $@

$(RELEASE_OBJS): | $(GEN_FILES)

$(RELEASE_BIN_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DEPDIR)/$^)
	$(CC) $(COMPILER_FLAGS) $(INCLUDE_FLAGS) $< -o $@
	$(POSTCOMPILE)

$(RELEASE_BIN_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(ASSEMBLER_FLAGS) $(INCLUDE_FLAGS) $< -o $@

$(RELEASE_BIN_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(ASSEMBLER_FLAGS) $(INCLUDE_FLAGS) $< -o $@

#
# Debug target
#

debug: BIN_DIR = $(DEBUG_BIN_DIR)
debug: MAP_FILE_PATH = $(DEBUG_MAP_FILE)
debug: BIN_FILE_PATH = $(DEBUG_BIN_FILE)
debug: ELF_FILE_PATH = $(DEBUG_ELF_FILE)
debug: $(DEBUG_BIN_FILE)

$(DEBUG_BIN_FILE): $(DEBUG_ELF_FILE)
	$(HEX) -O binary "$<" "$@"

$(DEBUG_ELF_FILE): $(DEBUG_OBJS)
	$(CC) $^ $(LINKER_FLAGS) -o $@

$(DEBUG_OBJS): | $(GEN_FILES)

$(DEBUG_BIN_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DEPDIR)/$^)
	$(CC) $(COMPILER_FLAGS) $(INCLUDE_FLAGS) $< -o $@
	$(POSTCOMPILE)

$(DEBUG_BIN_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(ASSEMBLER_FLAGS) $(INCLUDE_FLAGS) $< -o $@

$(DEBUG_BIN_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(ASSEMBLER_FLAGS) $(INCLUDE_FLAGS) $< -o $@


ifeq ($(BOARD_TYPE), $(filter $(BOARD_TYPE), NUCLEO_F7 F7))
   OPENOCD_FILE := target/stm32f7x.cfg
else ifeq ($(BOARD_TYPE), $(filter $(BOARD_TYPE), NUCLEO_F0 F0))
   OPENOCD_FILE := target/stm32f0x.cfg
else
	$(error "Unsupported Board type: $(BOARD_TYPE)")
endif

load: release
	$(eval OPENOCD_INSTANCES := $(shell (ps -ef | grep \"openocd\") | wc -l))
ifneq ("$(OPENOCD_INSTANCES)", "1")
	@echo "$(OPENOCD_INSTANCES) instances are open, make load may not compile"
endif
	openocd -f interface/stlink-v2-1.cfg -f $(OPENOCD_FILE) -c init -c "reset halt" -c halt -c "flash write_image erase $(RELEASE_BIN_FILE) 0x8000000" -c "verify_image $(RELEASE_BIN_FILE) 0x8000000" -c "reset run" -c shutdown

load-debug: debug
	openocd -f interface/stlink-v2-1.cfg -f $(OPENOCD_FILE) -c init -c "reset halt" -c halt -c "flash write_image erase $(DEBUG_BIN_FILE) 0x8000000" -c "verify_image $(DEBUG_BIN_FILE) 0x8000000" -c "reset run" -c shutdown

# Use this if you want gdb to be rtos thread aware
connect-rtos: load-debug
	openocd -f interface/stlink-v2-1.cfg -f $(OPENOCD_FILE) -c "stm32f7x.cpu configure -rtos FreeRTOS" -c init -c "reset halt" -c halt

# use this to debug stuff before rtos starts
connect: load-debug
	openocd -f interface/stlink-v2-1.cfg -f $(OPENOCD_FILE) -c init -c "reset halt" -c halt

#=======
#load: release
	## this is stand alone stlink
	## openocd -f interface/stlink-v2.cfg -f target/stm32f7x_stlink.cfg -c init -c "reset init" -c halt -c "flash write_image erase $(RELEASE_BIN_FILE) 0x08000000" -c "verify_image $(RELEASE_BIN_FILE)" -c "reset run" -c shutdown
	## this is for nucleo stlink
	#openocd -f interface/stlink-v2-1.cfg -f target/stm32f7x.cfg -c "reset_config srst_only connect_assert_srst" -c init -c "reset halt" -c halt -c "flash write_image erase $(RELEASE_BIN_FILE) 0x08000000" -c "verify_image $(RELEASE_BIN_FILE)" -c "reset run" -c shutdown


#connect: debug
	## this is stand alone stlink
	## openocd -f interface/stlink-v2.cfg -f target/stm32f7x_stlink.cfg -c init -c "reset init" -c halt -c "flash write_image erase $(RELEASE_BIN_FILE) 0x08000000" -c "verify_image $(RELEASE_BIN_FILE)" -c "reset run" -c shutdown
	## this is for nucleo stlink
	#openocd -f interface/stlink-v2-1.cfg -f target/stm32f7x.cfg -c init -c "reset init" -c halt -c "flash write_image erase $(RELEASE_BIN_FILE) 0x08000000" -c "verify_image $(RELEASE_BIN_FILE)" &
#>>>>>>> 7ce22243a5a59845c153b3d793ee480bc9de4175

# Need to start up openocd on seperate terminal using make connect
# If you use the same terminal for both connect and gdb, then sending ctrl-c to stop the program will also stop openocd so you will get kicked out of gdb (not what you want)
gdb:
	arm-none-eabi-gdb --eval-command="target remote localhost:3333" --eval-command="monitor reset halt" $(DEBUG_ELF_FILE)
	#arm-none-eabi-gdb --eval-command="target remote localhost:3333" --eval-command="monitor reset halt" --eval-command="monitor arm semihosting enable"  $(DEBUG_ELF_FILE)

clean:
	$(RM) $(BIN_BASE_DIR)
	$(RM) $(DEPDIR)
	$(RM) $(GEN_DIR)

docs:
	@[ $(DOCS_MAKEFILE_PATH) ] || (echo "Docs path is not set!"; exit 1)
	make -C $(DOCS_MAKEFILE_PATH) html
	@echo "Documentation generation complete. Location: $(DOCS_MAKEFILE_PATH)"

test:
	# No tests right now.
	# This is what it would look like if we did have tests:
	# make -C tests/ run

######################################
# Created autogenerated files
#######################################

ifeq ($(BOARD_TYPE), $(filter $(BOARD_TYPE), NUCLEO_F7 F7))
F0_OR_F7 := "F7"
else ifeq ($(BOARD_TYPE), $(filter $(BOARD_TYPE), NUCLEO_F0 F0))
F0_OR_F7 := "F0"
else
	$(error "Unsupported Board type: $(BOARD_TYPE)")
endif

autogen: $(GEN_FILES)

$(GEN_INC_DIR)/$(BOARD_NAME)_can.h: $(GEN_SRC_DIR)/$(BOARD_NAME)_can.c

$(GEN_SRC_DIR)/$(BOARD_NAME)_can.c: $(CAN_FILES_GEN_SCRIPT) $(DBC_FILE)
	@mkdir -p $(GEN_DIR)
	$(CAN_FILES_GEN_SCRIPT) $(BOARD_NAME) $(F0_OR_F7)

$(GEN_INC_DIR)/$(BOARD_NAME)_charger_can.h: $(GEN_SRC_DIR)/$(BOARD_NAME)_charger_can.c

$(GEN_SRC_DIR)/$(BOARD_NAME)_charger_can.c: $(GEN_SRC_DIR)/$(BOARD_NAME)_can.c

$(GEN_INC_DIR)/$(BOARD_NAME)_dtc.h: $(DTC_FILES_GEN_SCRIPT) $(DTC_FILE)
	@mkdir -p $(GEN_DIR)
	$(DTC_FILES_GEN_SCRIPT) $(BOARD_NAME)

#$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d


#
# Include dependencies
#
include $(wildcard $(patsubst %,$(DEPDIR)/%.d, $(basename $(SRC))))
include $(shell mkdir -p Gen) $(wildcard Gen/*.d)
