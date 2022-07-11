import argparse
import can
import cantools
import csv
import logging
import os
import pkg_resources
import traceback

from datetime import datetime

from wfe.connect.packet import CANPacket
from wfe.connect.connect import QueueDataPublisher

from wfe.util import default_dbc_path

today = datetime.now().strftime("%b_%d_%y-%H-%M-%S")
logs_folder = "/home/debian/can_monitor_logs"

# Create logs folder if it does not already exist
if not os.path.isdir(logs_folder):
    os.mkdir(logs_folder)

# Logging setup
wfe_log_filename = "{}/{}_wfe.log".format(logs_folder, today)
log_format = "%(asctime)s - %(levelname)s - %(message)s"
logging.basicConfig(filename=wfe_log_filename, format=log_format)
logging.getLogger().setLevel(logging.DEBUG)

# CSV setup
csv_filename = "{}/{}_wfe.csv".format(logs_folder, today)
csv_file_exists = os.path.isfile(csv_filename)
with open(csv_filename, "a") as csv_file:
    csv_writer = csv.writer(csv_file)
    if not csv_file_exists:
        csv_writer.writerow(["Timestamp", "Signal", "Value"])

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
            try:
                # can_bus.recv is blocking, is unblocked after timeout
                message = self.can_bus.recv(timeout)
                if message is None:
                    logging.info("Failed to receive a CAN message on {} for {} "
                            "seconds".format(self.interface, timeout))
                    # Try again
                    continue

                logging.debug("Message received: {}".format(message))

                # Dictionary of decoded values and their signals
                decoded_data = self.db.decode_message(
                    message.arbitration_id,
                    message.data
                )
                logging.debug("Decoded signals: {}".format(decoded_data))

                # Log to CSV file
                with open(csv_filename, "a") as csv_file:
                    csv_writer = csv.writer(csv_file)
                    for signal in decoded_data:
                        csv_writer.writerow([message.timestamp, signal, decoded_data[signal]])

            except Exception as e:
                tb_msg = "".join(traceback.format_exception(None, e, e.__traceback__))
                logging.error(tb_msg)

            # Not needed for competition

            # Add the frame_id back into the decoded data dict
            # can_packet = CANPacket({
            #    "timestamp": message.timestamp,
            #    "frame_id": message.arbitration_id,
            #    "signals": decoded_data
            # })

            # self.send(can_packet)


# Parsing command line arguments
def get_arguments():
    parser = argparse.ArgumentParser(description="Monitor CAN messages being sent on a bus and log them to a file.")
    parser.add_argument("-i", "--interface", dest="interface", type=str,
                        default="can1", help="Bus interface (default: can1)")
    parser.add_argument("-d", "--dbc", dest="dbc", type=str,
                        default=default_dbc_path(),
                        help="DBC file (default: /home/wfe/data/2018CAR.dbc)")
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
