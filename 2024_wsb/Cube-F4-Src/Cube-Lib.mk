# ------------------------------------------------
# Generic Makefile (based on gcc)
# 
# ------------------------------------------------

THIS_MAKEFILE_PATH := $(CUBE_F4_MAKEFILE_PATH)
######################################
# target
######################################
TARGET = 2024_WSB


######################################
# building variables
######################################
# debug build?
LIB_DEBUG = 1
# optimization
LIB_OPT = -O3


#######################################
# paths
#######################################

######################################
# source
######################################
# C sources
LIB_C_SOURCES =  \
Src/adc.c \
Src/can.c \
Src/dma.c \
Src/freertos.c \
Src/gpio.c \
Src/i2c.c \
Src/iwdg.c \
Src/main.c \
Src/tim.c \
Src/spi.c \
Src/usart.c \
Src/stm32f4xx_hal_msp.c \
Src/stm32f4xx_it.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc_ex.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_can.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c_ex.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_iwdg.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c \
Src/system_stm32f4xx.c \
Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c \
Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c \
Middlewares/Third_Party/FreeRTOS/Source/list.c \
Middlewares/Third_Party/FreeRTOS/Source/queue.c \
Middlewares/Third_Party/FreeRTOS/Source/timers.c \
Middlewares/Third_Party/FreeRTOS/Source/tasks.c \
Middlewares/Third_Party/FreeRTOS/Source/event_groups.c \
Middlewares/Third_Party/FreeRTOS/Source/croutine.c \
Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c 

LIB_C_SOURCES := $(addprefix $(THIS_MAKEFILE_PATH), $(LIB_C_SOURCES))

# ASM sources
LIB_ASM_SOURCES =  \
startup_stm32f417xx.s
LIB_ASM_SOURCES := $(addprefix $(THIS_MAKEFILE_PATH), $(LIB_ASM_SOURCES))


#######################################
# CFLAGS
#######################################
# cpu
CPU = -mcpu=cortex-m4

# fpu
FPU = -mfpu=fpv4-sp-d16

# float-abi
FLOAT-ABI = -mfloat-abi=hard

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# macros for gcc
# AS defines
LIB_AS_DEFS = 

# C defines
LIB_C_DEFS =  \
-DUSE_HAL_DRIVER \
-DSTM32F417xx


# AS includes
LIB_AS_INCLUDES =  \
-I/Inc

# C includes
LIB_C_INCLUDES =  \
Inc \
Drivers/STM32F4xx_HAL_Driver/Inc \
Drivers/STM32F4xx_HAL_Driver/Inc/Legacy \
Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F \
Drivers/CMSIS/Device/ST/STM32F4xx/Include \
Middlewares/Third_Party/FreeRTOS/Source/include \
Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS \
Drivers/CMSIS/Include
LIB_C_INCLUDES := $(addprefix $(THIS_MAKEFILE_PATH), $(LIB_C_INCLUDES))

LIB_C_INCLUDES := $(addprefix -I, $(LIB_C_INCLUDES))


# compile gcc flags
LIB_ASFLAGS = $(LIB_MCU) $(LIB_AS_DEFS) $(LIB_AS_INCLUDES) $(LIB_OPT) -Wall -fdata-sections -ffunction-sections -c

LIB_CFLAGS = $(LIB_MCU) $(LIB_C_DEFS) $(LIB_C_INCLUDES) $(LIB_OPT) -Wall -fdata-sections -ffunction-sections -c

ifeq ($(LIB_DEBUG), 1)
LIB_CFLAGS += -g -gdwarf-2
endif


# Generate dependency information
#LIB_CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"


#######################################
# LDFLAGS
#######################################
# link script
LIB_LDSCRIPT = $(THIS_MAKEFILE_PATH)/STM32F417VGTx_FLASH.ld

# libraries
LIB_LIBS = -lc -lm -lnosys 
LIB_LIBDIR = 
LIB_LDFLAGS = $(LIB_MCU) -specs=nano.specs -T$(LIB_LDSCRIPT) $(LIB_LIBDIR) $(LIB_LIBS) -Wl,--gc-sections

# *** EOF ***
