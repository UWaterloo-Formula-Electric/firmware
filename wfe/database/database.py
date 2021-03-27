import cantools
import logging
import os
import pathlib
import sqlite3
from datetime import datetime, timedelta

from wfe.database.schema import TableWriter
from wfe.connect.connect import QueueDataSubscriber
from wfe.connect.connect import CANPacket

logging.basicConfig()
logger = logging.getLogger()
logging.getLogger().setLevel(logging.INFO)

path = pathlib.Path(__file__).resolve().parent

class Database(QueueDataSubscriber):

    DEFAULT_DB_PATH = '/tmp/wfe-data/'

    INSERT = "INSERT INTO {} VALUES ({})"
    DEFAULT_DBC = os.path.join(path, "../../common-all/Data/2018CAR.dbc")

    def __init__(self, custom_name=None):
        super(Database, self).__init__()
        self.subscribe_to_packet_type('')

        date = datetime.today().strftime('%Y-%m-%d')
        if not os.path.exists(self.DEFAULT_DB_PATH):
            os.makedirs(self.DEFAULT_DB_PATH)

        if custom_name:
            self.path = self.DEFAULT_DB_PATH + custom_name
        else:
            self.path = self.DEFAULT_DB_PATH + 'can_data-{}.db'.format(date)

        tw = TableWriter(self.path)
        tw.parse_dbc(self.DEFAULT_DBC)

        logger.info("Connecting to database '{}'".format(self.path))
        self.conn = sqlite3.connect(self.path)
        self.c = self.conn.cursor()
        logger.info("Connected to database.")

        self.db = cantools.database.load_file(self.DEFAULT_DBC)
        logger.info("Loaded CAN DBC.")

    def run(self):
        logger.info("Listening for packets...")
        last_execute_time = datetime.now()
        period = timedelta(seconds=5)
        insert_count = 0
        while True:
            packet = self.recv()
            insert = self.generate_insert_statement_by_packet(packet)
            # Not executed immediately, must wait until commit
            try:
                self.c.execute(insert)
            except sqlite3.OperationalError as e:
                print(packet.data)
                raise e

            insert_count += 1

            time = datetime.now()
            # Commmit periodically to prevent overloading the server.
            if time >= last_execute_time + period:
                self.conn.commit()
                logger.info("Committing {} inserts".format(insert_count))

    def generate_insert_statement_by_packet(self, packet):
        if isinstance(packet, CANPacket):
            return self.can_data_insert(packet)
        else:
            raise Exception("Not a packet: {}".format(packet))

    def can_data_insert(self, can_packet):
        frame_id = can_packet.data['frame_id']
        try:
            message = self.db.get_message_by_frame_id(frame_id)
        except KeyError as e:
            # print("Frame ID not recognized: 0x{:X} "
            #       "May need to reload the database".format(frame_id))
            raise(e)

        signals = []
        if message.is_multiplexed():
            table_name, signals = self.multiplexed_signals(
                    can_packet, message)
        else:
            table_name, signals = self.signals(can_packet, message)

        # Add the timestamp to every message
        signals.append(str(can_packet.data['timestamp']))

        return self.INSERT.format(table_name, ','.join(signals))

    def multiplexed_signals(self, can_packet, message):
        signals = []
        for signal in message.signals:
            if signal.name in can_packet.data['signals']:
                if not signal.is_multiplexer:
                    # Only uses the id of the last multiplexer
                    # Fails if there are more than one multiplexed messages
                    multiplexer_id = signal.multiplexer_ids[0]
                    signal_data = can_packet.data['signals'][signal.name]
                    signals.append(str(signal_data))

        table_name = message.name + str(multiplexer_id)
        return table_name, signals

    def signals(self, can_packet, message):
        signals = []
        table_name = message.name
        for signal in message.signals:
            signal_data = can_packet.data['signals'][signal.name]
            # sqlite requires that strings are surrounded by quotes
            if isinstance(signal_data, str):
                signal_data = '"' + signal_data + '"'

            signals.append(str(signal_data))

        return table_name, signals


