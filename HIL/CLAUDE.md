# Working in HIL/

HIL is a bench-only board (see `README.md`), but its code should still read
as if it belongs in this codebase next to `bmu/`, `vcu/`, `pdu/`. A
maintainer familiar with those boards should not be able to tell HIL's
firmware was written differently.

**Important caveat**: `bmu`/`vcu`/`pdu` are not perfectly uniform — they
were clearly written by different people across different years, and it
shows (indentation varies, brace placement varies, some files are
snake_case, some are camelCase). This file distills the *majority* pattern
or the *most disciplined* example where the codebase itself is split, and
states that as the single target style for new HIL code, rather than
pretending the reference codebase has no drift. Where a rule below picks one
option over another, that's a deliberate call to give HIL one consistent
style going forward — not a claim that the other option never appears
elsewhere in this repo.

There is no `.clang-format` anywhere in this repo (checked — none exists),
so nothing will auto-enforce this. Read it back before treating a change as
done.

## Canonical files to pattern-match against

When unsure how something should look, go read the real thing rather than
guessing:
- `bmu/Src/userInit.c` — pre-RTOS init hook shape, error-checking pattern.
- `bmu/Inc/bsp.h` — BSP macro style, `#if IS_BOARD_F7 / #else #error` guard.
- `bmu/Src/mainTaskEntry.c` — trivial task shape/period pattern.
- `bmu/Src/controlStateMachine.c` + `common/Inc/state_machine.h` — FSM
  pattern, if HIL ever grows one.
- `vcu/Src/canReceive.c`, `pdu/Src/canReceive.c` — CAN RX handler shape
  (adapt to HIL's single `HIL_CAN_Rx_Handler`, since there's no generated
  per-message dispatch table here).
- `common/Src/userCan.c` — the CAN send/queue API HIL's own code should call
  (`sendCanMessage`), not raw `HAL_CAN_*` calls from application code.
- `bmu/Src/F7_Src/ltc_common.c` — if HIL grows its own chip driver (e.g. for
  the DAC/pot used to inject signals), this is the cleanest example of the
  driver-layer pattern (see "Driver-layer style" below).

## Formatting

- **Indentation: 4 spaces, no tabs.** This is the majority convention
  (`bmu/Src/batteries.c`, `bmu/Src/controlStateMachine.c`,
  `vcu/Src/brakeAndThrottle.c`, `pdu/Src/cooling.c` all use it). A couple of
  reference files (`vcu/Src/traction_control.c`, `endurance_mode.c`) are
  tab-indented and one (`bmu/Src/fanControl.c`) is 2-space — don't follow
  those, they're outliers.
- **Function braces on their own line** (`void foo()\n{`), matching
  `bmu/Src/userInit.c`, `bmu/Src/controlStateMachine.c`,
  `bmu/Src/batteries.c`, and what's already in `HIL/Src/*.c`. The reference
  codebase is genuinely ~50/50 split on this (`contactorControl.c` uses
  same-line braces) — this file picks next-line as HIL's standard so it's at
  least internally consistent.
- **`if`/`for`/`while`/`switch` braces on the same line** (`if (x) {`) —
  this is the one bracing rule that actually holds consistently across the
  reference codebase.
- **Space before the paren** in control statements: `if (x)`, not `if(x)`.
- Wrap a `switch` `case` body in its own `{ }` block when it has any local
  state, matching `bmu/Src/controlStateMachine.c`'s `case` blocks.
- No enforced line-length limit exists in this codebase, but wrap long
  expressions by hand for readability rather than letting a single line run
  very long (see `vcu/Src/brakeAndThrottle.c`'s manually-wrapped
  `map_range_float` calls for the expected shape).

## Naming conventions

- **Functions**: camelCase, verb-first (`getX`, `checkX`, `initX`,
  `sendCanMessage`). The reference codebase has snake_case pockets
  (`vcu/Src/traction_control.c`, `endurance_mode.c`) — treat those as
  outliers, not a second valid style, for new HIL code.
- **Locals/parameters**: camelCase.
- **Globals**: plain camelCase, no prefix (`maxChargeCurrent`,
  `regenEnabled`). Don't invent a `g_`-style prefix — only one file in the
  whole reference codebase uses one, it's not the norm.
- **Structs/typedefs**: prefer `typedef struct { ... } Foo_t;` (anonymous
  struct, `_t` suffix) — this is the more common of the two patterns found
  (`pdu/Inc/loadSensor.h`, `pdu/Inc/lvMeasure.h`).
- **Enums**: `_t` suffix, PascalCase type name. Enum *values*: ALL_CAPS for
  plain/flag enums (`BOTS_FAILED_BIT`); if HIL ever grows an FSM, use the
  `PREFIX_Mixed_Case` style from `bmu/Inc/controlStateMachine.h`
  (`STATE_Self_Check`, `EV_HV_Toggle`) instead of ALL_CAPS, matching that
  convention specifically for states/events.
- **Macros/`#define` constants**: ALL_CAPS, always. This is the one naming
  rule that is fully consistent across the entire reference codebase — no
  exceptions to look for here.
- **`static` for file-local helpers**: mark them `static`. The reference
  codebase is inconsistent about this (`vcu/Src/brakeAndThrottle.c` does it,
  `bmu/Src/batteries.c` mostly doesn't) — follow the cleaner example and do
  it consistently in HIL even though not everything upstream does.

## Header file conventions

- Include guard: `#ifndef FOO_H` / `#define FOO_H` (blank line between them
  is common but optional), closing with `#endif /* FOO_H */`. **Do not use a
  leading double underscore** (`__FOO_H`) — that's a reserved identifier.
  Several older files in this repo (including `common/sample-bsp.h` and the
  real boards' `bsp.h`) do use `__BSP_H`; `HIL/Inc/bsp.h` intentionally uses
  `HIL_BSP_H` instead, and new HIL headers should follow that, not the
  legacy pattern.
- Include ordering: no rule is enforced repo-wide. Default to: the file's
  own header first (for a `.c` file), then other project headers, then
  system/HAL headers — or just match whatever the file you're editing
  already does. Don't invent alphabetical ordering; only one file in the
  reference codebase does that and it's not the norm.
- Doxygen file-header block (`@file`/`@author`/`@brief`/`@details`): keep
  using it for new files, matching `bmu/Src/userInit.c` and
  `HIL/Src/userInit.c`/`mainTaskEntry.c`. Fine to omit for a very short,
  self-explanatory stub file.

## Function structure / error handling

- Return `HAL_StatusTypeDef` for anything that can fail, propagated with the
  standard idiom used throughout the reference codebase:
  ```c
  if (foo() != HAL_OK) {
      ERROR_PRINT("Failed to foo\n");
      return HAL_ERROR;
  }
  ```
- If HIL ever grows an FSM, its transition functions should return
  `uint32_t` (the next state), matching `bmu/Src/controlStateMachine.c` —
  that's a deliberately different convention from the `HAL_StatusTypeDef`
  rule above, scoped specifically to FSM transition functions.
- `// TODO: <description>` — capital TODO, colon, one space. The reference
  codebase has several inconsistent variants (`TODO -`, `Todo:`); standardize
  on this one for new code.
- Don't leave a function whose real behavior is commented out while its
  surrounding code (a log message, a caller, a name) implies it still works.
  The research pass found a real example of this upstream — a
  `motorOverheated()`-style function that still logs "Overheating!" but had
  its actual protective action commented out, silently doing nothing. If
  something is a stub, make that obvious (an explicit `// TODO:` and a
  visibly-a-stub return), don't leave a misleading half-implementation.

## Logging

`common/Inc/debug.h` provides `DEBUG_PRINT` / `DEBUG_PRINT_ISR` /
`ERROR_PRINT` / `ERROR_PRINT_ISR` / `CONSOLE_PRINT` / `COMMAND_OUTPUT`.
Important fact about the mechanism: `DEBUG_PRINT` and `ERROR_PRINT` expand
to the *exact same underlying macro* — there's no compiler-enforced
severity difference between them. The distinction is a convention we keep
by hand, so be deliberate about it:

- **`ERROR_PRINT`**: genuine failure paths only — a HAL call failed, a fault
  tripped, a sanity check failed.
- **`DEBUG_PRINT`**: routine informational/trace logging — state entered,
  action started, periodic status.
- **Always use the `_ISR` variant from interrupt context** (CAN RX
  callbacks, HAL callbacks). The reference codebase is fully disciplined
  about this — zero violations found across every `*_Callback` function in
  `bmu`/`vcu`/`pdu`. Keep that discipline in `HIL/Src/canReceive.c`.
- `CONSOLE_PRINT` is effectively unused in board code. If HIL ever adds CLI
  commands, use `COMMAND_OUTPUT` inside the command handler instead,
  matching `controlStateMachine_mock.c`'s style (e.g. `"<cmd> <args>:\r\n
  <description>\r\n"` for help text).
- **Message format**: capitalize the first letter, use "Failed to X"
  phrasing for failures, end with `\n`. (The reference codebase mixes `\n`
  and `\r\n` inconsistently — just use `\n` and be consistent within HIL.)
  Don't bother prefixing every message with a module/function name; the
  file/function context already tells you that — only a couple of reference
  files do heavy prefixing and it's not the norm.
- **Format specifiers**, matching the dominant reference-codebase pattern:
  - `uint32_t` → `%lu` (decimal).
  - Bitmasks/notification words → `0x%lX`.
  - Small register/byte values → `0x%x` or `%02X`.
  - Floats → bare `%f`, no precision needed.
  - 64-bit CAN signal values → `PRIu64` from `<inttypes.h>`, not `%llu`
    (one reference file uses both inconsistently — prefer `PRIu64`, it's the
    more portable choice already present in this codebase).
- Don't log every routine CAN RX message on a high-frequency bus. Reserve
  `HIL_CAN_Rx_Handler` logging for messages a given test specifically cares
  about, matching `bmu/Src/canReceive.c`'s restraint — not
  `vcu/Src/canReceive.c`'s pattern of logging every 100 Hz message, which is
  noisier than it needs to be.
- CAN send/inject functions should log on failure only, not on every
  successful send, matching `pdu/Src/loadSensor.c`'s pattern.

## Driver-layer style (if HIL grows its own hardware drivers)

If HIL ends up driving its own chip (e.g. a DAC or digital pot for
injecting sensor signals, similar in spirit to `testbed/HIL_Firmware`'s DAC
driver), follow the pattern from BMU's cell-monitor drivers:

- One low-level transfer helper per chip/bus (e.g. `xxx_spi_tx_rx()`) that
  toggles CS via a `bsp.h` macro and calls the HAL once; every higher-level
  driver function goes through it — never call `HAL_SPI_*`/`HAL_I2C_*`
  directly outside that one helper (see `bmu/Src/F7_Src/ltc_common.c`'s
  `spi_tx_rx`).
- Public driver API shape: `HAL_StatusTypeDef <prefix>_<verb>_<noun>(...)`
  for anything that can fail (`<prefix>_init`, `<prefix>_read_x`,
  `<prefix>_write_x`), with output data passed via an out-parameter pointer
  rather than returned by value.
- Register/command bytes as `#define`s, not enums or bitfield structs —
  matches every chip driver in this codebase.
- Cite the datasheet in a comment next to any nontrivial constant or bit
  position (e.g. `ltc_common.h`'s `VUV`/`VOV` derivation comments) — this is
  a genuinely useful habit the driver layer follows consistently.
- Give driver files a full Doxygen file header describing the physical chip
  (part number, what it does, how it's wired) — the driver layer is the
  most consistently well-documented part of this codebase; keep that up.

## Things NOT to "fix" without asking first

These are deliberate, documented in `README.md` — don't silently undo them:
- No `Gen/HIL/` directory, no DBC/DTC codegen (`DBC_CODEGEN = 0` in
  `board.mk`).
- No `common/Src/canHeartbeat.c`, `watchdog.c`, `generalErrorHandler.c`,
  `state_machine.c`, or `canReceiveCommon.c` linked in yet — see
  `Src/canHeartbeatStub.c`'s comment for what re-adding one requires.
- `Inc/bsp.h`'s handle/pin macros are placeholders until a real schematic
  and `Cube-F7-Src-respin/` exist.
