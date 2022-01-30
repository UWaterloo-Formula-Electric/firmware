import argparse
import can
import cantools
import logging
import pkg_resources

from datetime import datetime

from wfe.connect.packet import CANPacket
from wfe.connect.connect import QueueDataPublisher

from wfe.util import default_dbc_path

wfe_log_filename = "{}_wfe.log".format(datetime.now().strftime("%Y-%m-%d_%H-%M-%S"))
log_format = "%(asctime)s - %(levelname)s - %(message)s"
logging.basicConfig(filename=wfe_log_filename, format=log_format)
logging.getLogger().setLevel(logging.DEBUG)

class CanMonitor(QueueDataPublisher):

    DEFAULT_DBC = default_dbc_path()

    def __init__(self, interface="can1", dbc=DEFAULT_DBC):
        super(CanMonitor, self).__init__()

        self.interface=interface
        logging.info("Initializing CAN monitor on {}".format(self.interface))
        self.can_bus = can.interface.Bus(self.interface, bustype="socketcan")
        self.db = cantools.database.load_file(dbc)

        self.monitoring = True
        
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


# Parsing command line arguments
def get_arguments():
    parser = argparse.ArgumentParser(description="Monitor CAN messages being sent on a bus and log them to a file.")
    parser.add_argument("-i", "--interface", dest="interface", type=str,
                        default="can1", help="Bus interface (default: can1)")
    parser.add_argument("-d", "--dbc", dest="dbc", type=str,
                        default="../../../../common/Data/2018CAR.dbc",
                        help="DBC file (default: firmware/common/Data/2018CAR.dbc)")
    args = parser.parse_args()
    return args.interface, args.dbc


def main():
    bus_interface, dbc_file = get_arguments()

    # For DBC files that come with the package
    if pkg_resources.resource_exists(__name__, dbc_file):
        dbc_file = pkg_resources.resource_filename(__name__, dbc_file)

    logging.info("Using DBC: {}".format(dbc_file))

    monitor = CanMonitor(interface=bus_interface, dbc=dbc_file)
    monitor.monitor_bus()

if __name__ == "__main__":
    main()
