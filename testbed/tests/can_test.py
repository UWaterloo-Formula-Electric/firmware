import time
from testbeds.hil_testbed import teststand
from drivers.common_drivers.can_driver import HILBoard, VehicleBoard
import numpy as np
import tqdm


def test_steeringAngle(teststand):
    print(teststand.vehicle_boards)
    vcu_hil: HILBoard = teststand.hil_boards["vcu_hil"]
    vcu: VehicleBoard = teststand.vehicle_boards["vcu"]
    
    for angle in range(-100, 101, 5):
        vcu_hil.set_signal("Steering_raw", {"Steering_raw": np.interp(angle, [-100, 100], [0, 3300])})
        time.sleep(0.001)
        assert vcu_hil.get_signal("Steering_status")
        time.sleep(0.75)
        steerAngle = vcu.get_signal("SteeringAngle")
        assert abs(steerAngle-angle) <= 5
        