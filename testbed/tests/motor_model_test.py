import time
from testbed.drivers.common_drivers.can_driver import VehicleBoard
from testbed.drivers.motor_driver import MotorModel
from testbed.testbeds.hil_testbed import teststand


def test_speed_motor():
    """
    validates that the throttle percent is properly converted to rpm
    """
    # print(teststand.vehicle_boards)
    vcu: VehicleBoard = teststand.vehicle_boards["vcu"]

    motor = MotorModel()
    for speed in range(0, 5000, 5):
        data = {"SpeedMotorLeft": speed}
        motor.send_speed_feedback(True, data)
        time.sleep(0.75)
        speedFeedback = vcu.get_signal("SteerSpeedMotorLeftingAngle")
        assert abs(speedFeedback - speed) <= 5
