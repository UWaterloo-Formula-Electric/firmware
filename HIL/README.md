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

## Directory structure

```
HIL/
  Inc/
    bsp.h            Board pin/peripheral handle macros (placeholders - TODO once schematic exists)
    HIL_can.h         Hand-written interface contract userCan.c/userCanF7.c expect (stands in for a generated <board>_can.h)
    canReceive.h      Single CAN RX entry point for application-level message handling
    canInject.h       Application-level CAN message spoofing/injection
  Src/
    userInit.c        Pre-RTOS init hook (weak-linked from Cube-generated main.c)
    mainTaskEntry.c    Default heartbeat/blink task
    HIL_can.c          Implements the HIL_can.h contract (filters, RX dispatch, DTC stub)
    canReceive.c       Application-level CAN RX handling
    canInject.c        Application-level CAN message spoofing/injection
    canHeartbeatStub.c Minimal stand-in for common/Src/canHeartbeat.c's globals (see above)
  Cube-F7-Src-respin/  STM32CubeMX-generated project (placeholder - not generated yet, see its README)
  board.mk
  README.md            You are here
```

## Building

HIL is intentionally **not** included in the root `Makefile`'s `all`/board
list or CI. To build it directly (once `Cube-F7-Src-respin/` has a real
CubeMX project in it):

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

- `Cube-F7-Src-respin/` needs an actual STM32CubeMX project (see its
  README).
- `Inc/bsp.h`'s handle/pin macros are placeholders pending a real schematic.
- `Src/canInject.c` and `Src/canReceive.c` are empty shells - this is where
  the actual message-spoofing/response-checking logic for each test goes,
  hand-built against `common/Data/2024CAR.dbc`.
