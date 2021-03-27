import sys
import json
import heapq
import logging
import argparse

import time
import threading

import can
import cantools

from can_monitor import CanMonitor

logging.basicConfig()
logger = logging.getLogger()
logging.getLogger().setLevel(logging.ERROR)

class MessageItem:
    """
    Encapsulates Message object and the time it will be sent on the bus.
    Used to perform heap operations when pushing/popping messages.
    """

    def __init__(self, message, start):
        self.message = message
        self.start = start

    # Comparison operator (less than) used when pushing/popping on heap
    def __lt__(self, other):
        return self.start < other.start

class CanSimulator:
    """
    Used to simulate CAN messages being sent on a bus.
    """

    def __init__(self, bustype="socketcan", channel="vcan0", bitrate=500000):
        self.bus = can.interface.Bus(bustype=bustype, channel=channel, bitrate=bitrate)
        self.message_heap = []
        self.messages = []

    def load_scenario(self, dbc_file, json_file):
        try:
            db = cantools.database.load_file(dbc_file)
            json_obj = None
            with open(json_file) as jf:
                json_obj = json.load(jf)

            periodic_msgs = json_obj["periodic_messages"]
            one_time_msgs = json_obj["one_time_messages"]
            
            msg_types = ["periodic_messages", "one_time_messages"]
            for m_type in msg_types:
                for msg_obj in json_obj[m_type]:
                    arb_id = db.get_message_by_name(msg_obj["name"]).frame_id
                    data = db.encode_message(msg_obj["name"], msg_obj["signals"])
                    message = can.Message(arbitration_id=arb_id, data=data)
                    start = msg_obj["time"]
                    period = -1 if "period_ms" not in msg_obj else msg_obj["period_ms"]
                    self.messages.append((message, start, period))

        except (FileNotFoundError, KeyError) as e:
            logging.error(e)

    def run(self, duration=5000):
        if len(self.messages) == 0:
            logging.error("No messages to send! Will not run simulation.")
            return

        # Add messages to heap
        logging.info("Preparing message heap...")

        items = 0
        for msg_tuple in self.messages:
            msg = msg_tuple[0]
            start = msg_tuple[1]
            period = msg_tuple[2]

            # Add periodic message to heap
            if period > 0:
                for i in range(0, duration-start+1, period):
                    heapq.heappush(self.message_heap, MessageItem(msg, start+i))
                    items += 1
            # Add one time message to heap
            else:
                heapq.heappush(self.message_heap, MessageItem(msg, start))
                items += 1

        logging.info("Finished preparing message heap - added {} items".format(items))

        failed_msgs = 0
        logging.info("### STARTING SIMULATION ###")
        start = time.time()

        # Send messages by starting time
        while len(self.message_heap) > 0:
            msg_w = heapq.heappop(self.message_heap)
            msg = msg_w.message
            # Convert start time to seconds
            msg_start = msg_w.start / 1000
            current = time.time() - start
            while current < msg_start:
                remaining = msg_start - current
                time.sleep(remaining)
                current = time.time() - start
            try:
                self.bus.send(msg)
            except can.CanError as e:
                failed_msgs += 1
                logging.error("Message {} failed to send! Error message: {}".format(msg.arbitration_id, e))

        elapsed = time.time() - start
        desired = duration / 1000
        if elapsed < desired:
            time.sleep(desired - elapsed)

        time_elapsed = round(time.time() - start, 3)
        percent_failed = (failed_msgs / items) * 100
        logging.info("### FINISHED SIMULATION - duration: {} seconds - {}% of messages failed to send ###".format(time_elapsed, percent_failed))

# Parsing command line arguments
def get_arguments():
    parser = argparse.ArgumentParser(description="Run CAN Simulations with CAN monitor.")
    parser.add_argument("-d", "--dbc", dest="dbc", type=str,
                        default="common-all/Data/2018CAR.dbc",
                        help="DBC file (default: common-all/Data/2018CAR.dbc)")
    parser.add_argument("-j", "--json", dest="json", type=str,
                        default="json/heartbeat.json",
                        help="JSON file (default: json/heartbeat.json)")
    parser.add_argument("-t", "--time", dest="time", type=int, default=5000,
                        help="Duration of simulation in ms (default: 5000)")
    args = parser.parse_args()
    return args.dbc, args.json, args.time

if __name__ == "__main__":
    dbc_file, json_file, duration = get_arguments()

    can_simulator = CanSimulator()
    can_simulator.load_scenario(dbc_file, json_file)

    can_monitor = CanMonitor(interface="vcan0", dbc=dbc_file)

    # Create separate threads for simulator and monitor to run
    simulator_thread = threading.Thread(target=can_simulator.run, args=(duration,))
    monitor_thread = threading.Thread(target=can_monitor.monitor_bus, args=(duration/1000,))

    simulator_thread.start()
    monitor_thread.start()

    simulator_thread.join()
    # Wait for 500 ms for remaining messages to be received
    # on CAN monitor before stopping its thread
    time.sleep(0.5)
    can_monitor.monitoring = False
    monitor_thread.join()
    logging.info("CAN simulator and monitor threads have ended!")
