# firmware/testbed/tests/brakelight_test.py

import time
from testbeds.hil_testbed import teststand
from drivers.common_drivers.can_driver import HILBoard
from tqdm import tqdm

# Define constants for the threshold voltage when the brake light turns on
BRAKE_LIGHT_ON_THRESHOLD_VOLTAGE_1 = 833 # Lower threshold voltage
BRAKE_LIGHT_ON_THRESHOLD_VOLTAGE_2 = 841 # Upper threshold voltage

def test_brakelight(teststand):
    print(teststand.vehicle_boards)
    
    vcu_hil: HILBoard = teststand.hil_boards["vcu_hil"]
    # update pud -> pdu_hil
    pdu_hil: HILBoard = teststand.hil_boards["pdu_hil"]

    # Iterate over the entire voltage range
    for voltage in tqdm(range(0, 3301)):
    
        # Set the brake position voltage
        vcu_hil.set_signal("BrakePosition", {"BrakePosition": voltage}) # Reference to HIL.dbc line 69
        time.sleep(0.001)
        
        # Check the Brake Pos Status
        assert vcu_hil.get_signal("BrakePosStatus"), " the brake position status signal is not correctly received"  # Reference to HIL.dbc line 63
        
        time.sleep(0.06)
        
        # Retrieve the current power status of the brake light from the PDU HIL board
        PowBrakeLight = pdu_hil.get_signal("PowBrakeLight") # Reference to HIL.dbc line 50

        # Check if the voltage is within the range where the brake light should be off
        if 0 <= voltage <= BRAKE_LIGHT_ON_THRESHOLD_VOLTAGE_1:
            if PowBrakeLight:
                assert PowBrakeLight == 0, "Brake light should be off at this voltage level"
        
        # Handle the unstable state range (from 833 to 841), where the brake light might be either on or off
        elif BRAKE_LIGHT_ON_THRESHOLD_VOLTAGE_1 < voltage < BRAKE_LIGHT_ON_THRESHOLD_VOLTAGE_2:
            pass  # No specific assertion as the state is considered unstable
        
        # Check if the voltage is above the upper threshold where the brake light should be on
        elif voltage >= BRAKE_LIGHT_ON_THRESHOLD_VOLTAGE_2:
            # Assert that the brake light is indeed on
            assert PowBrakeLight > 0, "Brake light should be on at this voltage level"
