from drivers.common_drivers.driver import TestbedDriver
import can
from slash import logger
import slash

class CANListener(can.Listener):
    def __init__(self, interface_name):
        super().__init__()
        self.interface_name = interface_name
        self.callbacks = dict()
    def register_can_listener(self, can_id, func_callback):
        self.callbacks[can_id] = func_callback
    def on_message_received(self, msg):
        board_id = (msg.arbitration_id >> 8) & 0x07
        if board_id in self.callbacks:
            self.callbacks[board_id](msg)

class CANDriver(TestbedDriver):
    def __init__(self, name, can_interface, can_id):
        super().__init__(name)
        self.can_id = can_id
        self.can_interface = can_interface
        if self.can_interface not in slash.g.can_listeners:
            slash.g.can_listeners[self.can_interface] = CANListener(self.can_interface)
            can_bus = can.Bus(channel=self.can_interface, bustype="socketcan")
            can.Notifier(can_bus, [slash.g.can_listeners[self.can_interface]])
        slash.g.can_listeners[self.can_interface].register_can_listener(self.can_id, self.can_msg_rx)

    def can_msg_rx(self, msg):
        logger.info(f"RXed id: {msg.arbitration_id}")
        # TODO: Store the CAN data within the CANDriver object

    def get_can_signal(self, signal_name):
        # TODO: Add DBC connection for signal name parsing
        # TODO: Add signal formatting and parsing through DBC
        pass
