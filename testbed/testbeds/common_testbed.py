from typing import List
import slash
from can.interfaces.vector import VectorBus
import can
from testbed.drivers.common_drivers.can_driver import (
    VehicleBoard,
    HILBoard,
    CANListener,
)

class HWManifestItem:
    """
    Class to store information about a hardware board in the testbed manifest
    @name: Name of the board, must NOT have spaces
    @classObj: Class object of the board driver, ex: VCU
    """

    def __init__(self, name: str, can_id_enum):
        self.name: str = name
        self.can_id: int = can_id_enum.value

    def __repr__(self) -> str:
        return f"HWManifestItem(name={self.name}, can_id={self.can_id})"


class Testbed:
    vehicle_manifest: List[HWManifestItem] = []
    hil_manifest: List[HWManifestItem] = []

    def __init__(self):
        # Choose which to enable, useful for debugging individual buses
        setup_vehicle = True
        setup_hil = True

        # Vehicle Bus
        if setup_vehicle:
            slash.g.vehicle_bus = VectorBus(channel=1)
            assert slash.g.vehicle_bus is not None, "Vehicle bus not initialized"

            slash.g.vehicle_listener = CANListener(
                dtc_logger=slash.g.vehicle_dtc_logger
            )
            slash.g.vehicle_listener.disable()
            can.Notifier(slash.g.vehicle_bus, [slash.g.vehicle_listener])

            self.vehicle_boards = {}
            for board in self.vehicle_manifest:
                self.vehicle_boards[board.name] = VehicleBoard(board.name, board.can_id)
            slash.g.vehicle_listener.enable()

        # HIL Bus
        if setup_hil:
            slash.g.hil_bus = VectorBus(channel=0)
            assert slash.g.hil_bus is not None, "HIL bus not initialized"

            slash.g.hil_listener = CANListener()
            slash.g.hil_listener.disable()
            can.Notifier(slash.g.hil_bus, [slash.g.hil_listener])

            self.hil_boards = {}
            for board in self.hil_manifest:
                self.hil_boards[board.name] = HILBoard(board.name, board.can_id)
            slash.g.hil_listener.enable()


class MotorModel:
    """
    Class which models the CAN interactions of the motor
    """

    def __init__(self) -> None:
        self.throttle_percent = 0
        self.current_speed = 0

    def receive_throttle(self, frame):
        """
        recieves throttle percentage
        ID:       0x8100302         Msg Name:   VCU_Data
        Sig Name: ThrottlePercent   Length:     0:8
        """
        self.throttle_percent = frame.data[0]

    def convert_throttle_to_speed(self, frame):
        """
        Converts throttle percentage to rpm for left motor
        ID:       0x586FF72         Msg Name:   SpeedFeedbackLeft
        Sig Name: SpeedMotorLeft    Length:     0:16

        ID:        0x596FF71        Msg Name:   SpeedFeedbackRight
        Sig Name:  SpeedMotorRight  Length: 0:16
        """
        # self.current_speed_left = (frame["data"][0] + (frame["data"][1] << 8)) - 32768
        self.current_speed = (frame.data[0] + (frame.data[1] << 8)) - 32768

    
