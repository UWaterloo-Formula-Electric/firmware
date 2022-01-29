import unittest
import os

from .context import wfe

from wfe.database.database import Database
from wfe.connect.packet import Packet, CANPacket

class TestDatabase(unittest.TestCase):

    CAN_INSERT = "INSERT INTO BMU_stateBatteryHV VALUES (200,12,20,1583886503.351468)"
    CAN_MULTIPLEXED_INSERT = "INSERT INTO BMU_CellVoltage2 VALUES (4,5,6,1583886503.351468)"

    def setUp(self):
        # TODO: Get real data
        self.CAN_packet = CANPacket({
            'timestamp': 1583886503.351468,
            'frame_id': 0x8000201,
            'signals': {'PowerBatteryHV': 20, 'CurrentDCBatteryHV': 12, 'VoltageBatteryHV': 200}
        })
        self.d = Database('test')


    def test_CAN_insert_generated(self):
        insert = self.d.can_data_insert(self.CAN_packet)
        self.assertEqual(insert, self.CAN_INSERT)

    def test_CAN_packet_generated_by_packet(self):
        insert = self.d.generate_insert_statement_by_packet(self.CAN_packet)
        self.assertEqual(insert, self.CAN_INSERT)

    def test_packet_invalid_frame_id(self):
        packet = CANPacket({
            'frame_id': 0x123,
            'timestamp': 0.0,
            'signals': {}
        })

        with self.assertRaises(KeyError):
            insert = self.d.generate_insert_statement_by_packet(packet)

    def test_packet_invalid_signals(self):
        packet = CANPacket({
            'frame_id': 0x8000201,
            'timestamp': 0.0,
            'signals': {'PowerBattery': 20, 'BatteryHV': 12, 'VoltageBatteryHV': 200}
        })

        with self.assertRaises(KeyError):
            insert = self.d.generate_insert_statement_by_packet(packet)

    def test_packet_signals_ordered_correctly(self):
        reordered_CAN_packet = CANPacket({
            'timestamp': 1583886503.351468,
            'frame_id': 0x8000201,
            'signals': {'CurrentDCBatteryHV': 12, 'PowerBatteryHV': 20, 'VoltageBatteryHV': 200}
        })

        insert = self.d.generate_insert_statement_by_packet(reordered_CAN_packet)

    def test_multiplexed_CAN_packet(self):
        packet = CANPacket({
            'timestamp': 1583886503.351468,
            'signals': {'VoltageCellMuxSelect': 2, 'VoltageCell08': 5, 'VoltageCell07': 4, 'VoltageCell09': 6},
            'frame_id': 411042817
        })
        insert = self.d.generate_insert_statement_by_packet(packet)
        self.assertEqual(insert, self.CAN_MULTIPLEXED_INSERT)

    def tearDown(self):
        os.remove(self.d.path)

