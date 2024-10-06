# firmware/testbed/tests/brakelight_test.py

import time
from testbeds.hil_testbed import teststand
from drivers.common_drivers.can_driver import HILBoard

# Define constants for the threshold voltage when the brake light turns on
BRAKE_LIGHT_ON_MINIMUM_THRESHOLD_VOLTAGE = 833  # Lower threshold voltage
BRAKE_LIGHT_ON_MAXIMUM_THRESHOLD_VOLTAGE = 841  # Upper threshold voltage

# Define a constant for the assumed noise from the HIL board.
# When added to the maximum threshold voltage, it should sum to about a 20% expected value.
PDU_HIL_DAC_NOISE = 0.08

# Define constants for the bounds of the voltage range used to test the brake light
TEST_RANGE_MINIMUM_VOLTAGE = 0
TEST_RANGE_MAXIMUM_VOLTAGE = 3301

# Define a constant for waiting for the HIL board to respond
BRAKE_LIGHT_HIL_SIGNAL_DELAY = 0.001

# Define a constant for maintaining a delay between sending the brake signal to the VCU and reading the light status.
BRAKE_LIGHT_VCU_TO_PDU_DELAY = 0.06


def test_brakelight(teststand):
    vcu_hil: HILBoard = teststand.hil_boards["vcu_hil"]
    # update pdu -> pdu_hil
    pdu_hil: HILBoard = teststand.hil_boards["pdu_hil"]

    # Iterate over the entire voltage range
    for voltage in range(TEST_RANGE_MINIMUM_VOLTAGE, TEST_RANGE_MAXIMUM_VOLTAGE):

        # Set the brake position voltage
        # HIL.dbc: Message ID 2214855183, bits 0 to 12.
        vcu_hil.set_signal("BrakePosition", {"BrakePosition": voltage})
        time.sleep(BRAKE_LIGHT_HIL_SIGNAL_DELAY)

        # Check the Brake Pos Status
        # HIL.dbc: Message ID 2282098434, bit 1.
        vcu_hil_signal = vcu_hil.get_signal("BrakePosStatus")
        assert vcu_hil_signal, f"The brake position status signal {vcu_hil_signal} was not correctly received"

        time.sleep(BRAKE_LIGHT_VCU_TO_PDU_DELAY)

        # Retrieve the current power status of the brake light from the PDU HIL board
        # HIL.dbc: Message ID 2281901827, bit 1.
        PowBrakeLight = pdu_hil.get_signal("PowBrakeLight")

        # Check if the voltage is within the range where the brake light should be off
        if 0 <= voltage <= BRAKE_LIGHT_ON_MINIMUM_THRESHOLD_VOLTAGE and PowBrakeLight:
            assert PowBrakeLight == 0, f"Brake light should be off, PowBrakeLight = {PowBrakeLight}."

        # Check if the voltage is above the lower threshold plus a noise offset, where the brake light should be on
        elif voltage >= BRAKE_LIGHT_ON_MAXIMUM_THRESHOLD_VOLTAGE + PDU_HIL_DAC_NOISE:
            # Assert that the brake light is indeed on
            assert PowBrakeLight > 0, f"Brake light should be on, PowBrakeLight = {PowBrakeLight}."
