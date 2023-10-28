import time
from testbed.testbeds.common_testbed import MotorModel
from testbed.drivers.common_drivers.can_driver import HILBoard, VehicleBoard
from testbed.testbeds.hil_testbed import teststand


def test_speed_motor(teststand):
    """
    validates that the throttle percent is properly converted to rpm
    """
    # print(teststand.vehicle_boards)
    motor_left = MotorModel()
    motor_right = MotorModel()
    for throttle_percent in range(0, 4065, 1):
        motor_left.convert_throttle_to_speed(frame=throttle_percent)
        # vcu_hil.set_signal(
        #     "SpeedMotorLeft", {"SpeedMotorLeft": motor_left.current_speed}
        # )
        time.sleep(0.001)
        # assert vcu_hil.get_signal("SpeedMotorLeft")
        motor_right.convert_throttle_to_speed(frame=throttle_percent)
        # vcu_hil.set_signal(
        #     "SpeedMotorRight", {"SpeedMotorRight": motor_right.current_speed}
        # )
        time.sleep(0.001)
        # assert vcu.get_signal("SpeedMotorRight")
