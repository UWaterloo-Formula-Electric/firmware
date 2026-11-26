# VCU 2027 schematic mapping

Transcribed from the user-supplied Connector.SchDoc and Processor.SchDoc screenshots, dated 2026-10-17. This is schematic evidence, not confirmation of the assembled board or HIL harness. The bracketed sensor buses are expanded on those sheets.

| Analog input | Connector pin | Processor pin | Current firmware pin |
| --- | --- | --- | --- |
| Throttle A | J1A.3 | PA1 | PA3 |
| Throttle B | J1A.4 | PA2 | PA4 |
| Brake position | J1A.5 | PA3 | PA5 |
| Brake pressure | J1A.6 | PA4 | PA6 |
| Steering | J1A.8 | PA5 | PA7 |

Current firmware evidence: `Cube-F7-Src-respin/Inc/main.h` and `Cube-F7-Src-respin/Src/adc.c`, which assign ADC1 channels 3-7 to ranks 1-5. Resolve the mismatch in the revision-specific CubeMX configuration and generated firmware before validation on the 2027 board. The existing ADC-equivalent test targets cannot be treated as confirmed connector voltages.

J1A.7 is labelled Brake_Pressure_RAW with a note that it goes to the BMU. Do not confuse it with the VCU's pressure input on J1A.6.

| Wheel input | Connector pin | Processor pin |
| --- | --- | --- |
| Rear right Hall | J1A.1 | PA6 |
| Rear left Hall | J1A.2 | PA7 |
| Front right encoder | J1A.9 | PE9 |
| Front left encoder | J1A.10 | PE11 |

Current `Src/traction_control.c` getters obtain front speed from WSB CAN variables and rear speed from inverter feedback. These getters do not establish support for the physical inputs above. Confirm the WS conditioning, pulses per revolution, permitted levels/rates and missing-pulse behavior before implementing the HIL pulse tests.

| Button input | Connector pin | Processor pin | Current button task |
| --- | --- | --- | --- |
| HV | J2A.1 | PC4 | Handled |
| EM | J2A.2 | PC5 | Handled |
| TC | J2A.3 | PB0 | Handled |
| Endurance | J2A.4 | PB1 | Handled |
| Regen | J2A.5 | PB2 | Not handled |
| Cooling | J2A.6 | PE7 | Not handled |

The connector note specifies active-low button sense with pull-ups. Confirm the Dash Buttons conditioning and the intended Regen/Cooling actions before selecting a HIL drive circuit or treating their behavior as passing. Existing Endurance firmware toggles regen; do not assume that is the intended new button assignment.

CAN_N connects to J1B.12 and CAN_P to J1B.13. The processor-side CAN RX/TX nets connect through the transceiver to PB3/PB4, matching the current CAN3 pin configuration.

The five analog channels use the repeated Sensor_Signal_Conditioning sheet. Its assembly options select a divider (100 kΩ series, 200 kΩ to ground for the nominal 5 V option) or a 0 Ω series link with no lower resistor for the 3.3 V option, plus active/passive filter options. Confirm the actual population separately for every channel. The schematic supply labels alone do not prove which divider is fitted.

The selected upstream HIL DAC driver is available in HIL_2027 at commit 98b1298f13a70b03fd5c96eac65bf2cf516fd732. HIL output-to-VCU harness assignments, as-built component population and validation tolerances remain unconfirmed. Store independently validated expectations and tolerances once in `Inc/bench_test_definitions.h`, along with revision and confirmer, and keep the HIL profile expectations consistent.
