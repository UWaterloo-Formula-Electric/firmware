# HIL

A bench-only CAN test/simulation board (STM32F769BIT6). It never ships in
the car and is deliberately **not** wired into the root `Makefile`/CI board
list the way `bmu`, `pdu`, and `vcu` are, but it is the `HIL` node in
`common/Data/2024CAR.dbc`. Its job is to sit on a bench CAN bus and inject frames that
spoof messages from real vehicle boards (per `2024CAR.dbc`) so other boards
can be tested in isolation.

This is a different piece of infrastructure from `../testbed/HIL_Firmware/`
(the existing ESP32-based rig that injects *analog* signals into a board's
ADC pins). This board works at the CAN level instead.

## Same layout as `bmu`/`vcu`/`pdu`

HIL follows the same template as the vehicle boards: CubeMX-generated HAL
code kept separate from application code, a `board.mk` that includes
`common/tail.mk` for the toolchain, and CAN/DTC code generated at build time
from `common/Data/2024CAR.dbc` + `common/Data/DTC.csv` into `Gen/HIL/`.

What HIL sends and receives is set in the DBC, the same as any node:

- **Receive:** add `HIL` to the receiver list of a signal. The generated
  `parseCANData()` then decodes that message into per-signal globals and
  calls a `__weak CAN_Msg_<Msg>_Callback()`, which `Src/canReceive.c`
  overrides. See the comment at the top of that file for the format.
- **Send / spoof:** add `HIL` to the message's `BO_TX_BU_` line to get a
  generated `sendCAN_<Msg>()`. When spoofing a heartbeat, keep the real board
  first in that list, since the generator treats `senders[0]` as the
  heartbeat's owner. Don't spoof a board that is also on the bus, or both
  will transmit the same ID.

Adding `HIL` to a message doesn't change any other board's generated code,
since each board's codegen only looks at its own node name.

`Src/canReceive.c` overrides the generated `__weak configCANFilters()` with an
accept-all filter, since the generated one only accepts frames addressed to
HIL's node address or broadcast, and a test rig needs to see everything.

Codegen needs the `cantools` Python package, so build from the repo-root venv
(see Building).

`common/tail.mk` still supports `DBC_CODEGEN = 0` in a `board.mk` to skip
codegen, but HIL no longer uses it.

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
- `common/Src/generalErrorHandler.c` - `_handleError()`, which every
  `handleError()` call and `common/Inc/debug.h`'s print macros need. Its DTC
  macros come from the generated `Gen/HIL/Inc/HIL_dtc.h`, and its
  `DTC_Fatal_Callback()` is defined in `Src/canReceive.c` and declared in
  `Inc/bsp.h` (the generated header only declares it for boards that receive
  a DTC message). HIL's board ID is `ID_HIL` (0) in
  `common/Inc/boardTypes.h`.

### Errors and DTCs

HIL isn't a Nucleo, so `generalErrorHandler.c` takes its production path, the
same as the vehicle boards: on `handleError()` it turns on the error LED,
raises the `ERROR_HANDLER` and `HIL_ERROR` DTCs, calls
`DTC_Fatal_Callback()`, and then **keeps running**. It does not halt or print
the file name.

Every DTC goes out on the bus in the `HIL_DTC` frame through the generated
`sendCAN_HIL_DTC()`, the same as the vehicle boards. A `handleError()` sends
`ERROR_HANDLER` (code 53, data = the line `handleError()` was called from)
and then `HIL_ERROR` (code 73). No board lists itself as a receiver of
`HIL_DTC`, so boards under test ignore it. Watch for it on a CAN logger.

**Not used, on purpose:**
- No `common/Src/canHeartbeat.c` (HIL isn't part of the vehicle's heartbeat
  network). `common/Src/debug.c`'s CLI still references a few of its
  globals/functions unconditionally, so `Src/canHeartbeatStub.c` provides
  just enough of a stand-in to satisfy the linker - see the comment in that
  file for how to swap in the real thing later.
- No `common/Src/watchdog.c`, `state_machine.c`, or `canReceiveCommon.c` for
  now. All are generic enough to add later. `watchdog.c` also needs an IWDG
  enabled in CubeMX and `canHeartbeat.c` linked in.

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
115200 baud. `cliTask` runs the FreeRTOS+CLI interpreter; type `help` for the
full list.

`common/Src/debug.c` provides `heap`, `taskList`, `stats`, `reset`,
`version`, and the `heartbeat*` commands to every board for free. HIL adds
its own in `Src/hilCli.c`, registered by `hilCliInit()` from `userInit()`:

```
i2cScan    <bus>                     Scan bus 1-3, list responding 7 bit addresses
i2cRead    <bus> <addr> <reg>        Read one byte
i2cWrite   <bus> <addr> <reg> <val>  Write one byte
pwmInit    <channel>                 (Re)start PWM_8/9/10 at 0% duty cycle
pwmSetDuty <channel> <percent>       Set PWM_8/9/10 to a 0-100 duty cycle
pwmStop    <channel>                 Stop PWM output on PWM_8/9/10
gpio3v1Enable <0|1>                  Drive GPIO3V_1 (PB12) low/high
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
    canReceive.h       Getters for state tracked by the CAN callbacks
    canInject.h        Application-level CAN message spoofing/injection
    hilCli.h           HIL's own CLI commands
    spiBus.h           Generic transfer wrappers for the bench SPI buses
    i2cBus.h           Generic transfer wrappers for the bench I2C buses
    pwmBus.h           Generic duty cycle control for the bench PWM channels
  Src/
    userInit.c         Pre-RTOS init hook (weak-linked from Cube-generated main.c)
    mainTaskEntry.c    Main task: starts CAN, then idles (PB12 is CLI-driven, so no LED blink)
    canReceive.c       CAN_Msg_<Msg>_Callback() overrides, accept-all CAN filter, DTC_Fatal_Callback()
    canInject.c        Application-level CAN message spoofing/injection
    canHeartbeatStub.c Minimal stand-in for common/Src/canHeartbeat.c's globals (see above)
    hilCli.c           HIL CLI commands (I2C, PWM, GPIO3V_1)
    spiBus.c           SPI4/SPI5 transfer wrappers
    i2cBus.c           I2C1/I2C2/I2C3 transfer wrappers
    pwmBus.c           PWM_8/9/10 (TIM8 channels 1-3) duty cycle control
  Cube-F7-Src-respin/  STM32CubeMX-generated project (HIL_2026.ioc), plus a hand-derived Cube-Lib.mk
  board.mk
  README.md            You are here

Gen/HIL/               Generated at build time from 2024CAR.dbc / DTC.csv (gitignored)
  Inc/HIL_can.h, Inc/HIL_dtc.h, Src/HIL_can.c
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
source venv/bin/activate
make -f HIL/board.mk HIL
```

run from the repo root. The venv provides `cantools` for the CAN codegen
script. Without it the build fails with `No module named 'cantools'`. Relative paths inside `common/tail.mk` resolve
against the working directory, not the `-f` path, so this works the same as
`make bmu` does today.

If you later decide HIL should be part of the normal build/CI (e.g. to
compile-check it on every PR), add `include HIL/board.mk` to the root
`Makefile` and add `HIL` to a target that isn't `all` (since it doesn't ship
in the car).

## What's still a stub / TODO

- HIL isn't a receiver of any signal in `2024CAR.dbc` yet, so the generated
  `parseCANData()` ignores every frame and `Src/canReceive.c` has no
  callbacks. Likewise the only messages it can send through generated code
  are `HIL_DTC` and the UART-over-CAN frame.
- `Src/canInject.c` is still an empty shell for hand-built frames. Prefer
  adding `HIL` to a message's `BO_TX_BU_` and using the generated
  `sendCAN_<Msg>()` instead.
- `PWM_1`..`PWM_7` (PG2-PG8) are net-labelled on the schematic but have no
  timer alternate function on this part and are absent from the `.ioc`, so
  they can only ever be bit-banged GPIO. Only `PWM_8`/`PWM_9`/`PWM_10`
  (PC6/PC7/PC8, on TIM8) have real PWM macros in `bsp.h`.
- Only CAN3 has NVIC interrupts enabled. CAN1 and CAN2 are configured but
  will not receive until their RX0/RX1 interrupts are ticked in CubeMX.
- No device drivers yet for whatever hangs off the SPI/I2C buses or the four
  `LDAC_n` lines. Those go in per-device files that call `spiBus`/`i2cBus`
  rather than the HAL directly. The external DAC driver is the current
  onboarding task.
