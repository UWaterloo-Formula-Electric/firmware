import slash
from slash import logger
import can


class MotorModel:
    """
    A representation of the motors functions
    """

    def __init__(self):
        self._bus = slash.g.vehicle_bus
        self.db = slash.g.vehicle_db

    def _set_signal(self, message_name: str, signal_value_pair: dict) -> bool:
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

    def send_dc_link_fbk(self, left_side: bool, signal, payload: dict):
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

        self._set_signal(message, payload)

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
        self._set_signal(message, payload)

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
        self._set_signal(message, payload)

    def send_temp_motor_left(self, left_side: bool, payload: dict):
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
        self._set_signal(message, payload)

    def send_torque_feedback_left(self, left_side: bool, payload: dict):
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
        self._set_signal(message, payload)

    def flush_tx(self):
        """Flushing will delete older messages but some may be backed up in queue"""
        self._bus.flush_tx_buffer()
