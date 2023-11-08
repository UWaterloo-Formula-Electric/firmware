# firmware/testbed/tests/brakelight_test.py

import time
from testbeds.hil_testbed import teststand
from drivers.common_drivers.can_driver import HILBoard, VehicleBoard
from tqdm import tqdm

def test_steeringAngle(teststand):
    print(teststand.vehicle_boards)
    vcu_hil: HILBoard = teststand.hil_boards["vcu_hil"]
    # pdu_hil: HILBoard = teststand.hil_boards["pdu_hil"]
    pdu: VehicleBoard = teststand.vehicle_boards["pdu"]
    

    # Iterate over the entire voltage range
    for voltage in tqdm(range(0, 3301)):
    
        # Set the brake position voltage
        vcu_hil.set_signal("BrakePosition", {"BrakePosition": voltage}) # HIL.dbc L69
        time.sleep(0.001) # Small delay after setting the voltage
        
        # Check the brake pos status
        assert vcu_hil.get_signal("BrakePosStatus")  # HIL.dbc L63
        
        time.sleep(0.16)
        # get Amp in Brake Light
        BrakeLightStatus = pdu.get_signal("ChannelCurrentBrakeLight") # 2018CAR.dbc
    
        if BrakeLightStatus:
            BrakeLightStatus = float(BrakeLightStatus)

        # the threshold voltages should be 0.8445V
        if 0 <= voltage <= 833:
            # Brake light should be off for voltages in the range 0 to 844mV
            if BrakeLightStatus:
              assert BrakeLightStatus == 0
        elif voltage >= 838:
            # Brake light should be on for voltages in the range 845 to 3300mV
            assert BrakeLightStatus > 0
        else:
            continue
            
        # time.sleep(1)  # Optional delay between tests
