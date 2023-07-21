import time
from typing import List, Callable
import cantools
import slash
from can.interfaces.socketcan.socketcan import SocketcanBus
from can.interfaces.vector import VectorBus
import can
from drivers.common_drivers.can_driver import VehicleBoard, HILBoard, CANListener


class HWManifestItem:
    '''
    Class to store information about a hardware board in the testbed manifest
    @name: Name of the board, must NOT have spaces
    @classObj: Class object of the board driver, ex: VCU
    '''

    def __init__(self, name: str, can_id: int):
        self.name = name
        self.can_id = can_id

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

            slash.g.vehicle_listener = CANListener()
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