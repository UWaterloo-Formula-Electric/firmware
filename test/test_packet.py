import unittest

from packet import Packet, CANPacket, PacketSchemaException

class TestPacket(unittest.TestCase):

    def test_must_be_subclassed(self):
        with self.assertRaises(NotImplementedError):
            packet = Packet()


class TestCANPacket(unittest.TestCase):

    def test_normal_packet(self):
        data = {
            "timestamp": 1583600559,
            "frame_id": 2226717697,
            "signals": {"ChargeEN_State": 0}
        }

        packet = CANPacket(data)

    def test_bad_packet(self):
        with self.assertRaises(PacketSchemaException):
            data = {
                "timestamp": 1583600560,
                "signals": {"ChargeEN_State": 0}
            }
            packet = CANPacket(data)

