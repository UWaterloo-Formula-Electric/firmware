# VCU 2027 compatibility work

Status: the pipeline and test definitions exist, but the current VCU firmware has not been ported to the supplied 2027 schematic. These steps describe the remaining implementation and validation; they are not completed hardware fixes.

## 1. Port the analog inputs first

1. Create a 2027 CubeMX configuration from the current VCU project, preserving the existing board configuration.
2. Match the supplied processor sheet: Throttle A -> PA1, Throttle B -> PA2, brake position -> PA3, brake pressure -> PA4, steering -> PA5.
3. Configure ADC1 ranks 1-5 as channels 1-5 in that same order. The current code configures channels 3-7. Keep the DMA buffer order consistent with `brakeAndThrottle.h`.
4. Remove the old ADC assignments on PA6/PA7 before configuring those pins for rear wheel-speed inputs. Check other peripheral conflicts in CubeMX.
5. Regenerate the variant's HAL files and select that variant in the build. Build before flashing.
6. On the assembled board, verify each connector input independently against raw ADC readings. Measure both connector and filtered voltages and confirm the fitted divider/filter option. Only then validate the percentage/angle conversions and save their expectations/tolerances.

## 2. Add the two new button inputs

1. Configure Regen on PB2 and Cooling on PE7 with the input mode required by the Dash Buttons conditioning circuit. Existing HV/EM/TC/Endurance pins already match the supplied sheet.
2. Add BSP pin definitions and debounce state bits for the new inputs in `buttonTask.c`. Reuse the existing debouncer and press/release behavior.
3. Add the agreed actions/state-machine events and CAN outputs. Current Endurance toggles regen; do not silently give two buttons the same action without the intended 2027 behavior being defined.
4. Review the existing `cooling-button-feature` branch as implementation reference; its presence does not establish that it is the accepted 2027 design.
5. Test all six inputs: short pulses, valid presses, hold, release, repeat and simultaneous presses. Verify one action per accepted press and the required behavior in disabled/fault states.

## 3. Implement physical wheel-speed capture

1. Match the schematic: RR PA6, RL PA7, FR PE9, FL PE11. Check the WS conditioning sheet and available timer alternate functions before choosing input capture or interrupt handling.
2. Implement pulse measurement and a zero-speed timeout. Store independently confirmed pulses/revolution and conversion units; for a simple pulse train RPM = 60 * frequency / pulses_per_revolution.
3. Connect the measurements to the 2027 telemetry/control paths with an explicit source selection. Current front getters use WSB CAN data; current rear getters use inverter speed. Leaving those getters unchanged would not test the new physical inputs.
4. HIL tests must exercise each physical input independently, including no pulses and the intended valid frequency range, and compare the channel's reported speed and timeout behavior against saved definitions.

## 4. Use the latest inspected HIL driver

Selected source: [firmware/HIL_2027 at 98b1298f13a70b03fd5c96eac65bf2cf516fd732](https://github.com/UWaterloo-Formula-Electric/firmware/tree/98b1298f13a70b03fd5c96eac65bf2cf516fd732/HIL), verified live on 2026-11-19. This supersedes the earlier cached 124eb5c3 snapshot.

- External driver: `HIL/Src/mcp4728.c`; command: `mcpSet <chip 1..4> <A..D> <millivolts> [gain]`.
- Internal driver: `HIL/Src/dacBus.c`; command: `dacSet <1|2> <millivolts>`.
- CLI definitions and exact acknowledgment formats: `HIL/Src/hilCli.c`.
- The driver documents unity-gain outputs DAC_OUT3..13 and 1.5x output stages DAC_OUT14..18. Choose gain according to the physical output stage, not according to the desired voltage.
- The host profile records the full selected commit. Before outputs change, the runner requests `version` and requires that commit hash.

The HIL driver documents its own outputs, but it does not assign those outputs to the five VCU inputs. The located [VCU HIL Harness page](https://app.notion.com/p/3fd7185a54644a0ca5ca189ed238ff01) is an old harness task without a pin map. No verified 2027 cable map was found in the sources inspected. Use the VCU-side map in `2027-pin-map.md`, trace the actual HIL cable, and record each connection in the fixture profile. Do not treat arbitrary DAC channel assignments as existing wiring.

After confirming the physical connections and divider/filter options, fill the five-output idle and stimulus recipes with the actual commands and acknowledgment lines. Measure outputs, enter the validated expected readings/tolerances once, and save the board/fixture/firmware revisions. Then run the single pipeline invocation and retain its report and CAN evidence.
