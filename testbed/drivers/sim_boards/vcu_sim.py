from drivers.common_drivers.can_driver import CANDriver
class VCUSim(CANDriver):
    CAN_ID = 1
    def __init__(self, name, can_interface):
        super().__init__(name, can_interface, self.CAN_ID)
