import cantools
import slash
from slash import logger
from testbed.drivers.common_drivers.driver import TestbedDriver
from testbed.drivers.dtc_logger import DTCLogger
import can
from typing import Dict, Callable
from can.interfaces.socketcan.socketcan import SocketcanBus


class CANListener(can.Listener):
    def __init__(self, dtc_logger: DTCLogger = None):
        super().__init__()
        self.callbacks: Dict[int, Callable] = {}
        self.dtc_logger = dtc_logger
        self._enabled = False
        
    def register_callback(self, board_id, func_callback):
        assert board_id not in self.callbacks
        self.callbacks[board_id] = func_callback

    def on_message_received(self, msg):
        if not self._enabled:
            return
        board_id = msg.arbitration_id & 0xff
        
        priority_mask = 0b111 << 26
        is_dtc = (msg.arbitration_id & priority_mask) == 0
        
        if is_dtc and self.dtc_logger is not None:
            self.dtc_logger.log_dtc(msg)
        else:
            assert board_id in self.callbacks, f"{board_id} not in {self.callbacks}, can_id: {hex(msg.arbitration_id)}"
            self.callbacks[board_id](msg)

    def enable(self):
        self._enabled = True
    
    def disable(self):
        self._enabled = False



class CANDriver(TestbedDriver):
    def __init__(self, name, can_id):
        super().__init__(name)

        self.name = name
        self.can_id = can_id
        self._bus: SocketcanBus = NotImplemented
        self.db: cantools.db.Database = NotImplemented

        self.store = {}

    def can_msg_rx(self, msg):
        try:
            decoded_data = self.db.decode_message(
                msg.arbitration_id,
                msg.data,
                allow_truncated=True,
                decode_choices=False,
                decode_containers=False
            )
        except KeyError:
            # logger.warning(f"Message {msg.arbitration_id} not found in {self.name}")
            return
            

        for signal_name, signal_value in decoded_data.items():  # type: ignore
            self.store[signal_name] = signal_value

    def get_signal(self, signal_name):
        try:
            return self.store[signal_name]
        except KeyError:
            logger.warning(f"Signal {signal_name} not found in {self.name} {self.store.keys()}")
            return None
        
    def __repr__(self) -> str:
        return f"{self.name}({self.can_id})"


class VehicleBoard(CANDriver):
    def __init__(self, name, can_id):
        super().__init__(name, can_id)

        self._bus = slash.g.vehicle_bus
        self.db = slash.g.vehicle_db
        slash.g.vehicle_listener.register_callback(self.can_id, self.can_msg_rx)
        self.dtc_logger = slash.g.vehicle_dtc_logger # So the user can access in tests


class HILBoard(CANDriver):
    def __init__(self, name, can_id):
        super().__init__(name, can_id)

        self._bus = slash.g.hil_bus
        self.db = slash.g.hil_db
        slash.g.hil_listener.register_callback(self.can_id, self.can_msg_rx)

    def set_signal(self, message_name: str, signal_value_pair: dict) -> bool:
        try:
            data = self.db.encode_message(
                message_name, signal_value_pair)
        except KeyError:
            logger.warning(f"Message {message_name} not found in {self.db}")
            return False
        msg = self.db.get_message_by_name(message_name)

        msg = can.Message(arbitration_id=msg.frame_id, data=data)
        self._bus.send(msg)
        return True

    def flush_tx(self):
        '''Flushing will delete older messages but some may be backed up in queue'''
        self._bus.flush_tx_buffer()

class MotorModel(CANDriver):
    """
    A representation of the motors functions
    """
    def __init__(self, name, can_id):
        super().__init__(name, can_id)

        self._bus = slash.g.vehicle_bus
        self.db = slash.g.vehicle_db
        slash.g.vehicle_listener.register_callback(self.can_id, self.can_msg_rx)

    def set_signal(self, message_name: str, signal_value_pair: dict) -> bool:
        """
        sets the value of a signal within a message
        """
        try:
            data = self.db.encode_message(
                message_name, signal_value_pair)
        except KeyError:
            logger.warning(f"Message {message_name} not found in {self.db}")
            return False
        msg = self.db.get_message_by_name(message_name)

        msg = can.Message(arbitration_id=msg.frame_id, data=data)
        self._bus.send(msg)
        return True

    def flush_tx(self):
        '''Flushing will delete older messages but some may be backed up in queue'''
        self._bus.flush_tx_buffer()
