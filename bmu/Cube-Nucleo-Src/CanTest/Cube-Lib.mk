##########################################################################################################################
# File automatically-generated by tool: [projectgenerator] version: [2.30.0] date: [Tue Nov 27 10:54:34 CET 2018]
##########################################################################################################################

# ------------------------------------------------
# Generic Makefile (based on gcc)
#
# ChangeLog :
#	2017-02-10 - Several enhancements + project update mode
#   2015-07-22 - first version
# ------------------------------------------------

THIS_MAKEFILE_PATH := $(CUBE_NUCLEO_MAKEFILE_PATH)
######################################
# target
######################################
TARGET = CanTest


######################################
# building variables
######################################
# debug build?
LIB_DEBUG = 1
# optimization
LIB_OPT = -Og


#######################################
# paths
#######################################
# source path
LIB_SOURCES_DIR =  \
Application \
Drivers/STM32F7xx_HAL_Driver \
Application/User/.. \
Application/User/../Src \
Drivers/CMSIS \
Middlewares \
Middlewares/FreeRTOS \
Drivers \
Application/User

# firmware library path
LIB_PERIFLIB_PATH =

# Build path
LIB_BUILD_DIR = build

######################################
# source
######################################
# C sources
LIB_C_SOURCES =  \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc_ex.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash.c \
../Src/main.c \
../Src/gpio.c \
../Src/can.c \
../Src/dma.c \
../Src/freertos.c \
../Src/tim.c \
../Src/spi.c \
../Src/usart.c \
../Src/stm32f7xx_it.c \
../Src/stm32f7xx_hal_msp.c \
../Src/stm32f7xx_hal_timebase_tim.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_can.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cortex.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim_ex.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc_ex.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash_ex.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_gpio.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma_ex.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr_ex.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c_ex.c \
../Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_iwdg.c \
..//Src/system_stm32f7xx.c \
../Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c \
../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1/port.c \
../Middlewares/Third_Party/FreeRTOS/Source/list.c \
../Middlewares/Third_Party/FreeRTOS/Source/queue.c \
../Middlewares/Third_Party/FreeRTOS/Source/timers.c \
../Middlewares/Third_Party/FreeRTOS/Source/tasks.c \
../Middlewares/Third_Party/FreeRTOS/Source/event_groups.c \
../Middlewares/Third_Party/FreeRTOS/Source/croutine.c \
../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c  \
../Src/iwdg.c

LIB_C_SOURCES := $(addprefix $(THIS_MAKEFILE_PATH), $(LIB_C_SOURCES))

# ASM sources
LIB_ASM_SOURCES =  \
startup_stm32f767xx.s
LIB_ASM_SOURCES := $(addprefix $(THIS_MAKEFILE_PATH), $(LIB_ASM_SOURCES))


#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S
 
#######################################
# CFLAGS
#######################################
# cpu
LIB_CPU = -mcpu=cortex-m7

# fpu
LIB_FPU = -mfpu=fpv5-d16

# float-abi
LIB_FLOAT-ABI = -mfloat-abi=hard

# mcu
LIB_MCU = $(LIB_CPU) -mthumb $(LIB_FPU) $(LIB_FLOAT-ABI)

# macros for gcc
# AS defines
LIB_AS_DEFS =

# C defines
LIB_C_DEFS =  \
-DUSE_HAL_DRIVER \
-DSTM32F767xx


# AS includes
LIB_AS_INCLUDES =  \
../Inc
LIB_AS_INCLUDES := $(addprefix $(THIS_MAKEFILE_PATH), $(LIB_AS_INCLUDES))

LIB_AS_INCLUDES := $(addprefix -I, $(LIB_AS_INCLUDES))

# C includes
LIB_C_INCLUDES =  \
../Inc \
../Drivers/STM32F7xx_HAL_Driver/Inc \
../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy \
../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1 \
../Drivers/CMSIS/Device/ST/STM32F7xx/Include \
../Middlewares/Third_Party/FreeRTOS/Source/include \
../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS \
../Drivers/CMSIS/Include

LIB_C_INCLUDES := $(addprefix $(THIS_MAKEFILE_PATH), $(LIB_C_INCLUDES))

LIB_C_INCLUDES := $(addprefix -I, $(LIB_C_INCLUDES))


# compile gcc flags
LIB_ASFLAGS = $(LIB_MCU) $(LIB_AS_DEFS) $(LIB_AS_INCLUDES) $(LIB_OPT) -Wall -fdata-sections -ffunction-sections -c

LIB_CFLAGS = $(LIB_MCU) $(LIB_C_DEFS) $(LIB_OPT) -Wall -fdata-sections -ffunction-sections -c

ifeq ($(LIB_DEBUG), 1)
LIB_CFLAGS += -g -gdwarf-2
endif


# Generate dependency information
#LIB_CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"


#######################################
# LDFLAGS
#######################################
# link script
LIB_LDSCRIPT = $(THIS_MAKEFILE_PATH)/STM32F767ZITx_FLASH.ld

# libraries
LIB_LIBS = -lc -lm -lnosys
LIB_LIBDIR =
LIB_LDFLAGS = $(LIB_MCU) -specs=nano.specs -T$(LIB_LDSCRIPT) $(LIB_LIBDIR) $(LIB_LIBS) -Wl,--gc-sections

# *** EOF ***
