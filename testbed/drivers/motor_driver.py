import slash
from slash import logger
from testbed.drivers.common_drivers.can_driver import CANDriver, CANListener
import can


class MotorModel(CANListener, CANDriver):
    """
    A representation of the motors functions.
    Input:
        CurrentLimit(Left/Right)
        SpeedLimit(Left/Right)
        TorqueLimit(Left/Right)
        VoltageLimit(Left/Right)
    Output:
        DCLinkFbk(Left/Right)
        SpeedFeedback(Left/Right)
        TempInverter(Left/Right)
        TempMotor(Left/Right)
        TorqueFeedback(Left/Right)
    """

    def __init__(self):
        super().__init__(name=None, can_id=None)
        self._bus = slash.g.vehicle_bus
        self.db = slash.g.vehicle_db

        slash.g.vehicle_listener = CANListener(
            dtc_logger=slash.g.vehicle_dtc_logger
        )
        self._listener = slash.g.vehicle_listener
        self._listener.disable()
        can.Notifier(self._bus, [self._listener])
        self.start(start=True)

    def start(self, start: False):
        while start is True:
            message = self._bus.recv()
            if message is not None:
                print(f"Received message: {message}")

    def set_signal(self, message_name: str, signal_value_pair: dict) -> bool:
        """
        Sets and sends a CAN message by retrieving the message ID and encoding a message.
        Sends the message based on the CAN bus set in the class.
        """
        try:
            data = self.db.encode_message(message_name, signal_value_pair)
        except KeyError:
            logger.warning(f"Message {message_name} not found in {self.db}")
            return False
        msg = self.db.get_message_by_name(message_name)

        msg = can.Message(arbitration_id=msg.frame_id, data=data)

        slash.g.vehicle_listener.register_callback(msg.frame_id, msg)

        self._bus.send(msg)
        return True

    # Defining the messages which the motor can send out
    def send_dc_link_fbk(self, left_side: bool, payload: dict):
        """
        Sends either:
        1. DCLinkFbkLeft ID: 0x597FF71
        2. DCLinkFbkRight ID: 0x587FF72
        
        depending on the left_side bool.
        """
        if left_side is True:
            message = "DCLinkFbkLeft"
        else:
            message = "DCLinkFbkRight"

        self.set_signal(message, payload)

    def send_speed_feedback(self, left_side: bool, payload: dict):
        """
        Sends either:
        1. SpeedFeedbackLeft ID: 0x596FF71
        2. SpeedFeedbackRight ID: 0x586FF72
        depending on the left_side bool.
        """
        if left_side is True:
            message = "SpeedFeedbackLeft"
        else:
            message = "SpeedFeedbackRight"
        self.set_signal(message, payload)

    def send_temp_inverter(self, left_side: bool, payload: dict):
        """
        Sends either:
        1. TempInverterLeft ID: 0x599FF71
        2. TempInverterRight ID: 0x589FF72
        depending on the left_side bool.
        """
        if left_side is True:
            message = "TempInverterLeft"
        else:
            message = "TempInverterRight"
        self.set_signal(message, payload)

    def send_temp_motor(self, left_side: bool, payload: dict):
        """
        Sends either:
        1. TempMotorLeft ID: 0x598FF71
        2. TempMotorRight ID: 0x588FF72
        depending on the left_side bool.
        """
        if left_side is True:
            message = "TempMotorLeft"
        else:
            message = "TempMotorRight"
        self.set_signal(message, payload)

    def send_torque_feedback(self, left_side: bool, payload: dict):
        """
        Sends either:
        1. TorqueFeedbackLeft ID: 0x18000205
        2. TorqueFeedbackRight ID: 0x18000206
        depending on the left_side bool.
        """
        if left_side is True:
            message = "TorqueFeedbackLeft"
        else:
            message = "TorqueFeedbackRight"
        self.set_signal(message, payload)

    def flush_tx(self):
        """Flushing will delete older messages but some may be backed up in queue"""
        self._bus.flush_tx_buffer()
