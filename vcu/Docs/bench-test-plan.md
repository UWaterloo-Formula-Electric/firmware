# VCU bench validation

Scope confirmed: sensor inputs, CAN, buttons and fault behavior. LED and buzzer validation are outside this task. Physical validation is pending. Draft numeric values describe the current firmware and require independent confirmation on the new revision.

| What to test | How to test | What’s expected |
| --- | --- | --- |
| T01 CLI communication | During vcuTest run, verify that the VCU’s streamed response is readable. Reply pass/fail at the prompt. | Readable VCU pipeline response. Record transport response time against the agreed timeout. |
| T02 Firmware identification | The run displays version automatically. Save build date, branch, commit, board revision, fixture revision, calibration and operator in the session log. Reply pass/fail. | Reported build matches the image loaded. Record local modifications because the commit hash alone does not identify an uncommitted build. |
| T03 Accelerator A | Fixture: apply raw ADC-equivalent A values 2096, 1204/1205, 313. At each point reply ready at the run prompt. Keep B valid and matched. | Expected A percentages 0, approximately 50, 100. Three successful points required. External connector voltages/pins and measurement tolerance need released hardware confirmation. |
| T04 Accelerator B | Fixture: apply B values 2044, 2924, 3804 with matching A. Reply ready at each point. | Expected B percentages 0, 50, 100 within recorded tolerance. Three successful points required. |
| T05 Calculated throttle | Release both brake inputs. Apply agreeing A/B values for 0%, 50%, 100%. Reply ready at each point. | Expected average throttle 0%, 50%, 100%, with internal status THROTTLE_OK (0). A numerically correct reading with another status fails. |
| T06 Accelerator disagreement | Keep raw inputs inside valid ranges. Test independently calibrated A/B separation below 15 percentage points, at 15, and above 15. Reply read to sample throttle and status. On the isolated simulator also verify the running fault response. Record pass/fail after all cases. | Below 15 points is accepted. At/above 15, calculated throttle is zero and getThrottle prints FAIL. Capture DTC and state/CAN behavior during running operation. |
| T07 Accelerator range | Use fixture to test A at 283 and 2126 (accepted boundaries), then 282 and 2127; test B at 2014 and 3834, then 2013 and 3835. Match the other channel’s calibrated percentage. Reply read to sample throttle and status. Record all cases. | Boundary values pass the range check. Values just outside are rejected, calculated throttle is zero, and status is FAIL. Distinguish range rejection from A/B disagreement. |
| T08 Brake position | Fixture: apply ADC-equivalent position 246, 222, 198. Reply ready for the stored 0%, 50%, 100% checks; getBrake position is the corresponding display. | Position percentage is inverted relative to raw input. All three independently expected values must meet the recorded tolerance. |
| T09 Brake pressure | Fixture: apply ADC-equivalent pressure 418, 918/919, 1419. Reply ready for the stored 0%, approximately 50%, 100% checks. getBrake pressure reports percent. | All three pressure percentages match calibration. CAN FrontBrakePressure is PSI, a separate conversion: clamp((raw*4.95/4095 - 0.5)*625, 0, 2500). Compare both units explicitly. |
| T10 Combined brake | In current combined mode, apply position=50%, pressure=56% then position=20%, pressure=80%. Reply ready for each point; the run checks stored expectations 53 then 80. Repeat exact 10-point boundary manually if required. | At <=10 percentage points difference the result is their average. Beyond 10 points it is the larger value. Confirm configured brake source before testing a different build. |
| T11 Brake/accelerator overlap | Supply brake >40% and matched accelerator >25%. Reduce accelerator to exactly 5%, then below 5% with brake released. Verify zero/status and warning messages. Record pass/fail. Reply read to observe throttle, status and numeric state. | Overlap latches suppression: zero with THROTTLE_DISABLED (getThrottle prints FAIL). Exactly 5% does not clear it. Below 5% clears it and valid inputs recover OK. |
| T12 Steering | Apply ADC-equivalent steering 1, 2048, 4095. Reply ready to check stored -102, 0, 102 degree expectations for this code/configuration. Also compare physical angle against the fixture’s calibrated angle. | Current integer conversion is (raw-2048)/20, so endpoints are -102 and +102 degrees. Physical sensor calibration and allowable angle error require hardware confirmation. |
| T13 CAN transmission | Capture VCU_Data with an independent CAN adapter on the assigned bus. Decode with this checkout’s DBC. Check known input values and timestamps over a recorded capture window. Reply pass/fail. | Extended ID 0x08100302, DLC 6, 10 ms target interval. BrakePercent, FrontBrakePressure (PSI), SteeringAngle and ThrottlePercent decode correctly. Record timing tolerance; throttle telemetry is held zero outside EM-enable. |
| T14 CAN reception | Simulator: send BMU_HV_Power_State off/on/off and monitor numeric state, VCU_EM_State and output commands. Also test repeated PDU_ChannelStatus ON messages and inverter internal-state feedback. Reply pass/fail. | BMU HV state drives the documented transitions; EM requires separate enable prerequisites. Repeated ON feedback should remain ON. Record the existing repeated-status callback issue described below. |
| T15 Physical buttons | Exercise J2A pins 1-6 (HV, EM, TC, Endurance, Regen, Cooling), one at a time. Inputs are active low per the connector note. Test debounced press, hold, release and repeat in valid simulator states. Confirm the Dash Buttons conditioning before selecting an electrical drive method. | One event per valid press with the agreed action and no repeats while held. Current firmware handles four buttons only; Regen and Cooling behavior/support remain unconfirmed and cannot be marked passing without implementation. |
| T16 Enable conditions | Simulator only. Test one invalid condition at a time: HV off, brake pressure <=250 PSI, nonzero accelerator >=5%, brake <=15%, missing PDU or inverter acknowledgment. Then supply all valid conditions and handshakes. Reply pass/fail. | Invalid cases do not complete EM-enable. Valid case requires HV on, pressure >250 PSI, valid throttle <5%, brake >15%, and successful PDU/inverter handshakes. Record state, warning and outgoing CAN evidence. |
| T17 Disable and fault response | Simulator only. Separately test HV loss, fatal DTC, brake raw out-of-range, and brake position/pressure disagreement while EM-enabled. Reset/re-establish the starting state between cases. Capture state, DTCs and outgoing motor commands before entering pass/fail. | Active brake faults produce their fatal DTC/state. HV loss exits EM-enable. Check disabling commands independently on CAN; record any missing response as failure. Brake disagreement polarity is an open code finding below. |
| T18 Wheel-speed inputs | Apply calibrated pulses separately to J1A.1 RR, J1A.2 RL, J1A.9 FR, J1A.10 FL; test zero and known rates. Store pulse-to-speed conversion, rate limits and stopped-input timeout during initial validation. Confirm WS conditioning and electrical levels first. | Correct channel, direction if applicable and speed within stored limits; stopped/missing pulses follow the agreed behavior. Current traction-control getters use CAN/inverter data, so this does not yet prove the new physical inputs work. Missing support stays pending. |
| T19 Revision compatibility | Verify the supplied Connector/Processor mapping against the as-built board, ADC ranks, all six buttons, four wheel-speed paths and CAN. Confirm the fitted sensor divider/filter option and each HIL output connection. | All in-scope channels map correctly. Current ADC firmware uses PA3-PA7 but the schematic requires PA1-PA5. Resolve this mismatch and the new button/wheel-speed support before claiming 2027 validation. |

## Simplified command interface

Use `vcuTest run` to start the full ordered sequence and `vcuTest report` to print its retained results. `vcuTest help` lists these choices. The separate start/show/check/next/retry/pass/fail/skip/abort CLI subcommands have been removed; ready, read, pass/fail, skip and abort remain replies to an active run. Each new run clears the previous report, so save the transcript first.

The firmware pipeline is reduced from approximately 513 to 348 lines. Detailed procedures and expected behavior remain in the test-plan table instead of being duplicated as console text. Sensor conversion functions, stored expectations/tolerances and the HIL stimulus handshake are retained. Manual checks still require evidence and an explicit result; simplification does not make these unattended.

## Initial validation and stored definitions

Enter expected readings and tolerances manually once during the initial validation session, then keep them in the version-controlled definitions for subsequent automatic comparisons. Use independently known fixture inputs, circuit calculations and calibrated reference measurements; do not use the VCU's observed value as its own pass criterion. The person performing/confirming initial validation records their name and the board/fixture/calibration revision. A specific named confirmer has not been supplied.

The numeric sensor definitions are in `vcu/Inc/bench_test_definitions.h`. Its supplied values and zero tolerances are unvalidated placeholders. After confirming them, update the expectations, limits, revision and confirmer, mark the definitions validated, and rebuild. `vcuTest run` then uses those saved values; unvalidated definitions reject a run. Validate the initial readings using the existing read-only commands, then save the approved values here. This is configuration stored in source and flashed firmware, not automatic learning or an on-device calibration-save command.

Keep the HIL fixture profile's `expected` entries consistent with these definitions. The runner aborts if the firmware and profile disagree. CAN timing limits, button actions/debounce, wheel-speed conversion/timeouts and fault responses must likewise be recorded once in the corresponding test definitions before those checks can become unattended; their current manual steps are not automated by storing sensor tolerances.

## 2027 schematic findings

See `vcu/Docs/2027-pin-map.md` and the fixture template for the pin mapping transcribed from the supplied Connector and Processor screenshots. These documents establish the VCU-side mapping; the HIL output assignments and actual fitted divider/filter components are still missing.

Current firmware uses ADC1 channels/ranks 3,4,5,6,7 (PA3-PA7). The supplied processor sheet routes Throttle A/B, brake position/pressure and steering to PA1-PA5, respectively. The current button task supports four inputs, while the schematic adds Regen on PB2 and Cooling on PE7. Current wheel-speed getters use WSB CAN and inverter feedback, rather than proving the four new pulse inputs. These are unresolved compatibility checks, not passing tests. Do not treat the existing build as a completed 2027 firmware port.

## VCU bench pipeline

Run only with the VCU on an isolated fixture and simulated BMU/PDU/inverter interfaces. Complete visual and supply-rail checks first. The firmware continues normal background operation. Use hardware isolation before power-on, including during state-changing tests.

Test points are ADC-equivalent counts from the current firmware calibration. Set the actual fixture voltage only after checking the released connector pinout, signal-conditioning circuit and allowable voltage range. Record sensor and timing tolerances before starting. TBD items and skipped tests keep validation incomplete.

Save the console transcript and CAN capture with the board revision, fixture revision, firmware commit, dirty working-tree state, test date and operator. No physical-board validation has been performed.

## Running the pipeline

1. Build with make vcu in the team dev container. Flash the resulting VCU image using the team’s bench flashing procedure.
2. During initial validation, enter independently expected readings and approved sensor tolerances in vcu/Inc/bench_test_definitions.h. Record revision and confirmer, set VCU_BENCH_DEFINITIONS_VALIDATED to 1 only after validation, then rebuild. Future runs use vcuTest run with no numeric arguments. Expected readings and tolerances are edited once in the definitions, rather than entered at each run.
3. The command advances through T01-T19 automatically. It displays firmware identification itself. With a verified HIL profile, the host commands the HIL at each sensor point and sends ready automatically after acknowledgment and settling. Without HIL control, set the fixture and reply ready manually. Expected values are stored in the firmware. Numeric readings, limits and status are compared automatically.
4. For physical/simulator checks, complete every subcase and reply pass, fail or skip followed by Enter. Reply read to sample throttle/status and numeric VCU state during the active command. These are replies to the running pipeline. Separate CLI commands cannot execute until the invocation finishes.
5. A failed reading or a fail reply stops the sequence and prints the summary. A skip advances but prevents overall PASS. Reply abort to stop. A reply timeout or overlong reply stops with the current test pending. Firmware waits at most 120 seconds for each reply.
6. Direct UART terminals send replies normally. The updated beaglebone/app/wfe/uart_over_can.py client streams the run and forwards fixture replies over CAN. Its manual reply prompt aborts after 100 seconds of inactivity (30 seconds when HIL control is enabled), before the firmware timeout. Save the complete transcript and external measurements.
7. After the invocation finishes, vcuTest report shows retained results. P means pass, F fail, S skip and - pending. OVERALL PASS requires all 19 tests to pass. Save results before reset, start a new vcuTest run when ready; each run clears the previous result array. If a fatal test requires reset, retain each transcript and explicitly skip previously recorded cases in the restarted run. The restarted partial report remains incomplete.

## STM32 HIL_2027 input generation

The required flow is one `vcuTest run` invocation: host commands HIL -> HIL generates the fixture input -> HIL acknowledges -> input settles -> host sends `ready` -> VCU reads and checks the result -> next point. The host connects to VCU over CAN and HIL over its UART4 debug console at 115200 baud. This does not require a BeagleBone specifically; it requires a host with SocketCAN and the HIL serial adapter.

The host orchestration is implemented in `beaglebone/app/wfe/hil_fixture.py` and `uart_over_can.py`. Live GitHub inspection on 2026-11-19 found a newer HIL_2027 commit than the cached local ref: `98b1298f13a70b03fd5c96eac65bf2cf516fd732` (PR #124). This selected revision contains `HIL/Src/mcp4728.c`, `dacBus.c` and CLI commands `mcpSet`/`dacSet`. The earlier missing-driver finding applied to the stale local snapshot and is superseded. The fixture profile pins this commit, and the host verifies the flashed HIL's version before changing outputs. A 2027 HIL-to-VCU cable map is still unconfirmed; the located VCU HIL Harness Notion page is a 2024 placeholder without pin assignments.

1. Build/flash the selected HIL_2027 commit with its DAC driver. Confirm all five output paths, voltage limits and transfer functions against the actual HIL-to-VCU harness. The host checks the full HIL commit before issuing stimulus commands.
2. Copy `vcu/Docs/hil-2027-fixture.template.json` to a fixture-specific JSON file. Fill the idle recipe and all 20 sensor-point recipes with actual HIL commands and their exact complete acknowledgment lines (`command` and `expect`). Each recipe must establish all five sensor inputs, including matched throttle channels and appropriate brake inputs. ADC-equivalent targets in the template are reference values, not DAC codes or connector voltages.
3. Record the fixture revision, measured settling times and independently expected VCU readings. Verify every output using external measurement, then set `verified` to true. The supplied incomplete template intentionally refuses to run before board I/O.
4. With `python-can`, `cantools` and `pyserial` installed in the host environment, start the console from the repository root:

   ```sh
   python3 beaglebone/app/wfe/uart_over_can.py --channel can1 --hil-port /dev/ttyUSB0 --hil-profile /path/to/verified-fixture.json
   ```

   The adapter path, CAN channel and profile path are bench-specific. Select `vcu>` once, then enter `vcuTest run` once. The firmware uses the saved, validated test definitions.

5. The runner matches stable Txx/Px identifiers, checks the profile's expected reading against the VCU request, executes the recipe sequentially, requires each acknowledgment and HIL prompt, waits for settling, then sends ready. A missing/rejected acknowledgment or unknown point sends abort. Commands are not retried automatically.
6. The configured idle recipe runs before the sequence, before manual checks and after completion, abort or failure. An idle restoration failure is reported and leaves validation incomplete; successful CLI acknowledgments alone do not prove the output voltage.

Only the sensor-point handoff is automated by this addition. Fault subcases, CAN simulation, buttons, wheel-speed inputs and revision-specific channels still need their fixture controls and measurement support; they retain explicit manual results. They must not be automatically marked PASS. VCU firmware/harness changes and physical validation are still outstanding. The selected DAC driver is available upstream but its physical operation on this fixture is not validated.

Verification: the VCU build and simplified CLI checks pass for draft definitions, validated definitions and invalid stored limits; 12 HIL host tests pass, including rejection of a mismatched HIL firmware commit before output changes. Host tests cover complete-profile checks, exact HIL acknowledgment matching, split UART prompts, missing acknowledgments, expectation mismatch, abort forwarding over CAN and idle restoration. These use simulated interfaces and do not validate hardware.

## Current-code findings to resolve before sign-off

Brake plausibility checking is active in InvCommandTask when combined-brake mode is enabled and the VCU is EM-enabled. The __unused annotation on its definition does not disable the call. The fault checker converts position without the inversion used by getBrakePotentiometerPercent, so investigate the polarity discrepancy on the simulator.

checkBPSState currently returns true unconditionally. An electrical brake-pressure fault detector is not implemented by that function. Record this as a known firmware gap, not a passing diagnostic.

PDU_ChannelStatus turns its cached motor-controller state off on a second ON message. Repeated-status testing should expose this existing callback defect.

Existing brake/throttle/throttleAB CLI setters overwrite the live ADC buffer and may be overwritten by DMA. The throttle-A mock conversion also has different polarity from the real read conversion. The new pipeline uses physical fixture inputs and existing read functions instead of those setters.

Two existing hv command definitions share the same name with different argument counts. Use physical buttons and simulated incoming CAN for these tests until that ambiguity is resolved.

The state-name array had a missing comma between HV Enable and Em Enable. This implementation corrects the display mapping so the existing state command can be used during validation.

## Software verification

VCU firmware builds with ARM GNU Toolchain 14.2.rel1 and warnings treated as errors. The existing linker RWX-segment warning remains. The simplified pipeline passed host checks through the real FreeRTOS CLI parser with address/undefined-behavior sanitizers and stub sensor/RTOS interfaces, covering full runs, repeated runs, skipped/failed checks, invalid replies, abort, timeout, overflow and bad stored limits. Transport checks cover prompt forwarding, ordinary commands, split console prompts, timeout, Ctrl-C and EOF. Physical board and CAN wiring validation remains pending.

## Sources

Command definitions and pipeline: vcu/Src/drive_by_wire_mock.c

Sensor conversion and safety checks: vcu/Src/brakeAndThrottle.c and vcu/Inc/brakeAndThrottle.h

State transitions and fault behavior: vcu/Src/drive_by_wire.c and vcu/Inc/drive_by_wire.h

CAN callbacks and signals: vcu/Src/canReceive.c and common/Data/2024CAR.dbc

Button behavior: vcu/Src/buttonTask.c

New hardware scope: user-supplied Connector.SchDoc, Processor.SchDoc and Sensor_Signal_Conditioning.SchDoc screenshots dated 2026-10-17. As-built population, HIL output mapping and acceptance tolerances still require confirmation.

## Compatibility fixes

See `vcu/Docs/2027-compatibility-fixes.md` for the ordered ADC, button, wheel-speed and HIL integration work. The selected upstream DAC driver resolves the source-availability gap; it does not resolve the VCU port or prove harness wiring.
