import can
import cantools
import json
import logging
import zmq

from datetime import datetime

from wfe.connect.packet import CANPacket
from wfe.connect.connect import QueueDataPublisher

from wfe.util import default_dbc_path

logging.basicConfig()
logging.getLogger().setLevel(logging.ERROR)

class CanMonitor(QueueDataPublisher):

    DEFAULT_DBC = default_dbc_path()

    def __init__(self, interface='can1', dbc=DEFAULT_DBC, log_to_file=True):
        super(CanMonitor, self).__init__()

        self.interface=interface
        logging.info("Initializing CAN monitor on {}".format(self.interface))
        self.can_bus = can.interface.Bus(self.interface, bustype='socketcan')
        self.db = cantools.database.load_file(dbc)

        self.monitoring = True

        if log_to_file:
            wfe_log_filename = "{}_wfe.log".format(datetime.now().strftime("%Y-%m-%d_%H-%M-%S"))
            logging.basicConfig(filename=wfe_log_filename)
        
    def monitor_bus(self, timeout=60):
        while self.monitoring:
            # can_bus.recv is blocking, is unblocked after timeout
            message = self.can_bus.recv(timeout)
            if message is None:
                logging.info("Failed to receive a CAN message on {} for {} "
                        "seconds".format(self.interface, timeout))
                # Try again
                continue

            logging.debug("Message received: {}".format(message))

            try:
                # Dictionary of decoded values and their signals
                decoded_data = self.db.decode_message(
                    message.arbitration_id,
                    message.data
                )

            except KeyError:
                # Generally, if this happens the DBC is out of date.
                logging.warning("Message ID {} could not be decoded".format(
                    message.arbitration_id
                ))
                # Try again
                continue

            logging.debug("Decoded signals: {}".format(decoded_data))

            # Add the frame_id back into the decoded data dict
            can_packet = CANPacket({
                "timestamp": message.timestamp,
                "frame_id": message.arbitration_id,
                "signals": decoded_data
            })

            self.send(can_packet)


def main():
    monitor = CanMonitor()
    monitor.monitor_bus()

if __name__ == "__main__":
    main()

