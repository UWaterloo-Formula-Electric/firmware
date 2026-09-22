##########################################################################################################################
# Hand-adapted from the CubeMX-generated Makefile in this directory (STM32CubeMX "Makefile" toolchain output).
# See ../../common/sample-Cube-Lib.mk for the template this follows, and any of the other boards'
# Cube-F7-Src-respin/Cube-Lib.mk for a worked example of the same transformation.
#
# Re-derive this file by hand whenever the .ioc is regenerated and the plain Makefile in this
# directory changes (new/removed peripherals, etc.) - CubeMX does not produce this file itself.
##########################################################################################################################

THIS_MAKEFILE_PATH := $(CUBE_F7_MAKEFILE_PATH)
######################################
# target
######################################
TARGET = HIL


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
# Build path
LIB_BUILD_DIR = build

######################################
# source
######################################
# C sources
LIB_C_SOURCES =  \
Core/Src/main.c \
Core/Src/gpio.c \
Core/Src/freertos.c \
Core/Src/can.c \
Core/Src/dac.c \
Core/Src/i2c.c \
Core/Src/spi.c \
Core/Src/tim.c \
Core/Src/usart.c \
Core/Src/stm32f7xx_it.c \
Core/Src/stm32f7xx_hal_msp.c \
Core/Src/stm32f7xx_hal_timebase_tim.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim_ex.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_can.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc_ex.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash_ex.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_gpio.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma_ex.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr_ex.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cortex.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c_ex.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_exti.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dac.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dac_ex.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi_ex.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart.c \
Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart_ex.c \
Core/Src/system_stm32f7xx.c \
Middlewares/Third_Party/FreeRTOS/Source/croutine.c \
Middlewares/Third_Party/FreeRTOS/Source/event_groups.c \
Middlewares/Third_Party/FreeRTOS/Source/list.c \
Middlewares/Third_Party/FreeRTOS/Source/queue.c \
Middlewares/Third_Party/FreeRTOS/Source/stream_buffer.c \
Middlewares/Third_Party/FreeRTOS/Source/tasks.c \
Middlewares/Third_Party/FreeRTOS/Source/timers.c \
Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c \
Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c \
Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1/port.c \
Core/Src/sysmem.c

# NOTE: Core/Src/syscalls.c is deliberately EXCLUDED here (unlike the rest of
# CubeMX's generated C_SOURCES list above) - it redefines _read/_write/_close/
# _kill/_isatty/_getpid/_fstat with real (but different) signatures than
# common/Src/newlibHack.c's stubs of the same names, which every board links
# in via COMMON_LIB_SRC. bmu/vcu/pdu's Cube-Lib.mk exclude their equivalent
# generated syscalls.c/sysmem.c for the same reason. sysmem.c only provides
# _sbrk(), which nothing else defines, so it's kept.

LIB_C_SOURCES := $(addprefix $(THIS_MAKEFILE_PATH), $(LIB_C_SOURCES))

# ASM sources
LIB_ASM_SOURCES =  \
startup_stm32f769xx.s
LIB_ASM_SOURCES := $(addprefix $(THIS_MAKEFILE_PATH), $(LIB_ASM_SOURCES))

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
-DSTM32F769xx


# AS includes
LIB_AS_INCLUDES =  \
Core/Inc
LIB_AS_INCLUDES := $(addprefix $(THIS_MAKEFILE_PATH), $(LIB_AS_INCLUDES))

LIB_AS_INCLUDES := $(addprefix -I, $(LIB_AS_INCLUDES))

# C includes
LIB_C_INCLUDES =  \
Core/Inc \
Drivers/STM32F7xx_HAL_Driver/Inc \
Drivers/STM32F7xx_HAL_Driver/Inc/Legacy \
Middlewares/Third_Party/FreeRTOS/Source/include \
Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS \
Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1 \
Drivers/CMSIS/Device/ST/STM32F7xx/Include \
Drivers/CMSIS/Include

LIB_C_INCLUDES := $(addprefix $(THIS_MAKEFILE_PATH), $(LIB_C_INCLUDES))

LIB_C_INCLUDES := $(addprefix -I, $(LIB_C_INCLUDES))


# compile gcc flags
LIB_ASFLAGS = $(LIB_MCU) $(LIB_AS_DEFS) $(LIB_AS_INCLUDES) $(LIB_OPT) -Wall -fdata-sections -ffunction-sections -c

LIB_CFLAGS = $(LIB_MCU) $(LIB_C_DEFS) $(LIB_OPT) -Wall -fdata-sections -ffunction-sections -c

ifeq ($(LIB_DEBUG), 1)
LIB_CFLAGS += -g -gdwarf-2
endif

# Dependency generation is handled by common/tail.mk instead
#LIB_CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"


#######################################
# LDFLAGS
#######################################
# link script
LIB_LDSCRIPT = $(THIS_MAKEFILE_PATH)/stm32f769bitx_flash.ld

# libraries
LIB_LIBS = -lc -lm -lnosys
LIB_LIBDIR =
LIB_LDFLAGS = $(LIB_MCU) -specs=nano.specs -T$(LIB_LDSCRIPT) $(LIB_LIBDIR) $(LIB_LIBS) -Wl,--gc-sections
