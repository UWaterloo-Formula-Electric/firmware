# HIL

A bench-only CAN test/simulation board (STM32F769BIT6). It never ships in
the car, is not a node in `common/Data/2024CAR.dbc`, and is deliberately
**not** wired into the root `Makefile`/CI board list the way `bmu`, `pdu`,
and `vcu` are. Its job is to sit on a bench CAN bus and inject frames that
spoof messages from real vehicle boards (per `2024CAR.dbc`) so other boards
can be tested in isolation.

This is a different piece of infrastructure from `../testbed/HIL_Firmware/`
(the existing ESP32-based rig that injects *analog* signals into a board's
ADC pins). This board works at the CAN level instead.

## Why this looks almost, but not quite, like `bmu`/`vcu`/`pdu`

The rest of the firmware repo's boards share a common template (see the
architecture notes from the earlier discussion): CubeMX-generated HAL code
kept separate from application code, a `board.mk` that includes
`common/tail.mk` for the toolchain, and CAN/DTC code generated at build time
from `common/Data/2024CAR.dbc` + `common/Data/DTC.csv`.

HIL keeps the **first two** (directory layout, `board.mk` + `common/tail.mk`)
but deliberately skips the **third** (DBC/DTC codegen), because:

- It isn't a real vehicle CAN node, so it has no natural entry in
  `common/Data/2024CAR.dbc` and no `Gen/HIL/` output.
- Its whole purpose is to *hand-construct* frames that impersonate other
  boards' messages, which the generated `sendCAN_<Msg>()` accessors (scoped
  to messages a *given* node sends) can't do anyway.

To make that possible, `board.mk` sets `DBC_CODEGEN = 0` (a flag added to
`common/tail.mk` for this purpose - it defaults to `1` so every other board
is unaffected). With that flag off, `common/tail.mk` no longer tries to run
`common/Scripts/generateCANHeadder.py`/`generateDTC.py` or add a
`Gen/HIL/Src/HIL_can.c` to the build.

`board.mk` also sets `MCU_DEFINE = STM32F769xx` (another small,
backward-compatible addition to `common/tail.mk` - it previously hardcoded
`STM32F767xx` for every F7 board; it now defaults to that but can be
overridden), since HIL uses a different part in the F7 family than
`bmu`/`vcu`/`pdu`.

## What's reused from `common/`, and what isn't

Per the decision made when scaffolding this board:

**Reused as-is:**
- `common/tail.mk` - toolchain, linker flags, `load`/`connect`/`gdb`/`clean`
  targets, git-metadata-in-binary, everything else every board gets.
- `common/Src/userCan.c` + `common/f7/Src/userCanF7.c` - the actual CAN
  driver plumbing (init, priority-queued TX, mailbox management, RX ISR
  dispatch). This is the one piece of "CAN architecture" HIL does share,
  because reimplementing reliable CAN TX/RX isn't worth it.
- `common/Src/debug.c` + `common/Src/FreeRTOS_CLI.c` - UART debug
  printing/CLI.
- `common/Src/freertos_openocd_hack.c` + `common/Src/newlibHack.c` - boring
  boilerplate every FreeRTOS+newlib-nano board needs.

**Not used, on purpose:**
- No DBC/DTC codegen (`Gen/HIL/` does not exist) - see above.
- No `common/Src/generalErrorHandler.c` (its `SEND_FATAL_DTC`/
  `SEND_CRITICAL_DTC` macros need a generated per-board `HIL_dtc.h` that
  doesn't exist). HIL doesn't yet have its own error handler beyond
  `Error_Handler()`/`vApplicationStackOverflowHook()` in `Src/userInit.c` -
  add one in `Src/errorHandler.c` if/when you need more than that.
- No `common/Src/canHeartbeat.c` (HIL isn't part of the vehicle's heartbeat
  network). `common/Src/debug.c`'s CLI still references a few of its
  globals/functions unconditionally, so `Src/canHeartbeatStub.c` provides
  just enough of a stand-in to satisfy the linker - see the comment in that
  file for how to swap in the real thing later.
- No `common/Src/watchdog.c`, `state_machine.c`, or `canReceiveCommon.c` for
  now. All are generic enough to add later; `watchdog.c` and
  `canHeartbeat.c` do reference a `BOARD_ID`/`ID_HIL`, so if you add them,
  add `#define ID_HIL <n>` to `common/Inc/boardTypes.h` first.

## FreeRTOS tasks

Declared in the CubeMX task table (**FREERTOS -> Tasks and Queues**) and
emitted into `Cube-F7-Src-respin/Core/Src/freertos.c`, the same as every
vehicle board. To add or retune a task, open CubeMX - not a source file.

| Task | Priority | Stack | Entry function | Code generation |
|---|---|---|---|---|
| `defaultTask` | Normal | 256 | `defaultTaskFunction` | Default |
| `mainTask` | Normal | 1000 | `mainTaskFunction` | As external |
| `printTaskName` | Low | 1000 | `printTask` | As external |
| `cliTaskName` | Low | 1000 | `cliTask` | As external |
| `canSendTask` | Realtime | 1000 | `canTask` | As external |

Two things to know before touching this table:

**Code Generation must stay `As external` for the four real tasks.** Their
bodies live in `Src/mainTaskEntry.c` and `common/Src/{debug,userCan}.c`.
Setting one back to `Default` makes CubeMX generate a second body for the
same symbol in `freertos.c`. That does not fail the build, because
`common/tail.mk` passes `-z muldefs` to the linker, which allows duplicate
symbols and silently keeps whichever comes first in link order. The stub
CubeMX generates is `for(;;) { osDelay(1); }`, so if it ever won you would
get a board that boots and does nothing - no CLI, no debug output, no CAN,
no error message anywhere.

**`defaultTask` cannot be removed.** CubeMX requires at least one task and
greys out both Delete and Code Generation for it. It runs an `osDelay(1)`
loop, costing ~1 KB of the 50 KB heap and a 1 ms wakeup that immediately
sleeps. Harmless - `defaultTaskFunction` is defined only in `freertos.c`, so
it collides with nothing. Do not try to repurpose it by pointing it at
`mainTaskFunction`: its Code Generation is locked to `Default`, so that would
generate a duplicate `mainTaskFunction` body and hit exactly the problem
described above.

One consequence of using CubeMX for this: the generated code ignores
`osThreadCreate` return values, so a task that fails to spawn (heap
exhaustion) silently never runs. With four 1000-word tasks against a 50 KB
heap there is plenty of margin, but that is the trade versus creating tasks
in application code.

## CLI

Connect a serial adapter to the debug UART (`DEBUG_UART_HANDLE`, `UART4`) at
230400 baud. `cliTask` runs the FreeRTOS+CLI interpreter; type `help` for the
full list.

`common/Src/debug.c` provides `heap`, `taskList`, `stats`, `reset`,
`version`, and the `heartbeat*` commands to every board for free. HIL adds
its own in `Src/hilCli.c`, registered by `hilCliInit()` from `userInit()`:

```
i2cScan    <bus>                     Scan bus 1-3, list responding 7 bit addresses
i2cRead    <bus> <addr> <reg>        Read one byte
i2cWrite   <bus> <addr> <reg> <val>  Write one byte
pwmSetDuty <channel> <percent>       Set PWM_8/9/10 to a 0-100 duty cycle
pwmStop    <channel>                 Stop PWM output on PWM_8/9/10
```

`<addr>` is the 7 bit address printed in the device datasheet - these
commands apply the `<< 1` the HAL expects, so pass `0x60`, not `0xC0`. Every
value except `<bus>` is hex.

These are deliberately device agnostic so a chip can be exercised before any
driver exists for it, which is the intended bring-up path:

1. `i2cScan 1` - confirm the part ACKs. Proves wiring, pull-ups, address and
   bus number before any code is written.
2. `i2cRead` / `i2cWrite` - poke registers by hand against the datasheet.
3. Write the driver in its own file, calling `i2cBus.h` rather than the HAL.
4. Add a command here to exercise it, next to these.

`taskList` is also worth knowing: it prints each task's minimum free stack,
which is how you would justify the 1000-word stack sizes rather than leaving
them at a round guess.

## Directory structure

```
HIL/
  Inc/
    bsp.h              Board pin/peripheral handle macros, from the .ioc + Altium schematic
    HIL_can.h          Hand-written interface contract userCan.c/userCanF7.c expect (stands in for a generated <board>_can.h)
    canReceive.h       Single CAN RX entry point for application-level message handling
    canInject.h        Application-level CAN message spoofing/injection
    hilCli.h           HIL's own CLI commands
    spiBus.h           Generic transfer wrappers for the bench SPI buses
    i2cBus.h           Generic transfer wrappers for the bench I2C buses
    pwmBus.h           Generic duty cycle control for the bench PWM channels
  Src/
    userInit.c         Pre-RTOS init hook (weak-linked from Cube-generated main.c)
    mainTaskEntry.c    Main task: starts CAN, then blinks the debug LED
    HIL_can.c          Implements the HIL_can.h contract (filters, RX dispatch, DTC/UART-over-CAN stubs)
    canReceive.c       Application-level CAN RX handling
    canInject.c        Application-level CAN message spoofing/injection
    canHeartbeatStub.c Minimal stand-in for common/Src/canHeartbeat.c's globals (see above)
    hilCli.c           i2cScan / i2cRead / i2cWrite / pwmSetDuty / pwmStop CLI commands
    spiBus.c           SPI4/SPI5 transfer wrappers
    i2cBus.c           I2C1/I2C2/I2C3 transfer wrappers
    pwmBus.c           PWM_8/9/10 (TIM8 channels 1-3) duty cycle control
  Cube-F7-Src-respin/  STM32CubeMX-generated project (HIL_2026.ioc), plus a hand-derived Cube-Lib.mk
  board.mk
  CLAUDE.md            Code style rules for this directory
  README.md            You are here
```

`Cube-F7-Src-respin/Cube-Lib.mk` is hand-derived from the CubeMX-generated
`Makefile` next to it - CubeMX does not produce it. `common/tail.mk` includes
it to pick up the HAL/FreeRTOS source list and flags. Every board has one and
they are all maintained the same way. It only needs updating if a
regeneration adds a peripheral *type* the board never used before (a new
`Core/Src/<periph>.c` + its `stm32f7xx_hal_<periph>.c`); adding another timer
or ADC channel lands in existing files and changes nothing.

## Building

HIL is intentionally **not** included in the root `Makefile`'s `all`/board
list or CI. To build it directly:

```
make -f HIL/board.mk HIL
```

run from the repo root (relative paths inside `common/tail.mk` resolve
against the working directory, not the `-f` path, so this works the same as
`make bmu` does today).

If you later decide HIL should be part of the normal build/CI (e.g. to
compile-check it on every PR), add `include HIL/board.mk` to the root
`Makefile` and add `HIL` to a target that isn't `all` (since it doesn't ship
in the car).

## What's still a stub / TODO

- `Src/canInject.c` and `Src/canReceive.c` are empty shells - this is where
  the actual message-spoofing/response-checking logic for each test goes,
  hand-built against `common/Data/2024CAR.dbc`.
- `PWM_1`..`PWM_7` (PG2-PG8) are net-labelled on the schematic but have no
  timer alternate function on this part and are absent from the `.ioc`, so
  they can only ever be bit-banged GPIO. Only `PWM_8`/`PWM_9`/`PWM_10`
  (PC6/PC7/PC8, on TIM8) have real PWM macros in `bsp.h`.
- Only CAN3 has NVIC interrupts enabled. CAN1 and CAN2 are configured but
  will not receive until their RX0/RX1 interrupts are ticked in CubeMX.
- No device drivers yet for whatever hangs off the SPI/I2C buses or the four
  `LDAC_n` lines. Those go in per-device files calling `spiBus`/`i2cBus`,
  per `CLAUDE.md`'s driver-layer section. The external DAC driver is the
  current onboarding task.
- `SPI5` is still configured as 4 bit frames at 50 MHz (`SPI4` was corrected
  to 8 bit / 1.5625 MHz). Harmless while nothing uses SPI5, but set Data Size
  and Prescaler to match `SPI4` in CubeMX before anyone does.
