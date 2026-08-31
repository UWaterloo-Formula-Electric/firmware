# Cube-F7-Src-respin (placeholder)

This directory is where the STM32CubeMX/STM32CubeIDE-generated project for
the HIL board's MCU (**STM32F769BIT6**) belongs, mirroring every other
board's `Cube-F7-Src-respin/` (see e.g. `../../bmu/Cube-F7-Src-respin/`).

It is not generated yet - this README is a stand-in so the directory exists
in git. To bring the board up:

1. Create a new STM32CubeMX project targeting **STM32F769BIT6**.
2. Enable FreeRTOS (same CMSIS-RTOS API version the other F7 boards use -
   check `../../bmu/Cube-F7-Src-respin/*.ioc`), CAN1, USART2 (or whatever the
   real debug UART ends up being), the LED GPIOs referenced in
   `../Inc/bsp.h`, and IWDG.
3. Generate the project *into this directory* so it produces the usual
   `Drivers/`, `Middlewares/`, `Inc/`, `Src/`, a linker script
   (`STM32F769BITx_FLASH.ld`), the startup assembly, the `.ioc` file, and a
   `Cube-Lib.mk` - the same shape `common/sample-Cube-Lib.mk` documents.
4. Update `../Inc/bsp.h`'s handle/pin `#define`s to match the real
   peripheral instances CubeMX assigned.
5. `../board.mk` already points `CUBE_F7_MAKEFILE_PATH` at this directory, so
   once `Cube-Lib.mk` exists here, `make -f HIL/board.mk HIL` (run from the
   repo root) should build.
