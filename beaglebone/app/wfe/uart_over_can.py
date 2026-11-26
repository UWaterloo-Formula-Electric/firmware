import traceback
import argparse
import re
import select
import sys
from pathlib import Path

import can.interfaces.socketcan
import can
import cantools
try:
    from util import default_dbc_path, bcolors
except ModuleNotFoundError as error:
    if error.name != "util":
        raise

    # The monorepo checkout does not contain the legacy BeagleBone util module.
    def default_dbc_path():
        return Path(__file__).resolve().parents[3] / "common" / "Data" / "2024CAR.dbc"

    class bcolors:
        OKCYAN = "\033[96m"
        OKBLUE = "\033[94m"
        BOLD = "\033[1m"
        FAIL = "\033[91m"
        WARNING = "\033[93m"
        ENDC = "\033[0m"


class UartOverCanInterface():

    DEFAULT_DBC = default_dbc_path()

    def __init__(self, channel="can1", dbc=DEFAULT_DBC):
        self.db = cantools.db.load_file(dbc)
        bitrate = 500_000
        self.timeout = 1  # seconds
        self.channel = channel

        print(f"Initializing CAN monitor on {self.channel}")
        self.can_bus = can.interface.Bus(self.channel, bustype="socketcan", can_filters=[
            {"can_id": 0x40602FF, "can_mask": 0xFFFFFFF, "extended": True}])  # type: ignore

    def send(self, msg_type, data):
        if msg_type == "config":
            tx_msg = self.db.get_message_by_name("UartOverCanConfig")
            msg_data = tx_msg.encode({"UartOverCanConfigSignal": data})
            msg = can.Message(arbitration_id=tx_msg.frame_id, data=msg_data)
            self.can_bus.send(msg)
        elif msg_type == "data":
            tx_msg = self.db.get_message_by_name("UartOverCanTx")
            for c in data:
                msg_data = tx_msg.encode({"UartOverCanTX": ord(c)})
                msg = can.Message(
                    arbitration_id=tx_msg.frame_id, data=msg_data)
                self.can_bus.send(msg, self.timeout)
        else:
            raise can.CanError("Message type invalid")

    def recv(self):
        # can_bus.recv is blocking, is unblocked after timeout
        msg = self.can_bus.recv(self.timeout)
        if msg is None:
            raise can.CanError(f"Failed to receive a CAN message on {self.channel} for {self.timeout} "
                               "seconds")
        # Dictionary of decoded values and their signals
        decoded_data = self.db.decode_message(
            msg.arbitration_id,
            msg.data
        )

        return decoded_data["UartOverCanRX"]

    def fast_recv(self):
        # can_bus.recv is blocking, is unblocked after timeout
        msg = self.can_bus.recv(self.timeout)
        if msg is None:
            raise can.CanError(f"Failed to receive a CAN message on {self.channel} for {self.timeout} "
                               "seconds")
        return msg.data

    def fast_single_response(self, filters):
        msg_size = self.fast_recv()[0]

        msg_bytes = bytearray()
        i = 0
        while i < msg_size:
            msg_bytes.extend(self.fast_recv())
            i += 4
        for exclusion_filter in filters:
            if exclusion_filter in msg_bytes:
                return bytearray()
        return msg_bytes

    def fast_full_response(self, msg, cli_tag, filters, reply_reader=None, prompt_reader=None):
        '''
        This function is a faster verison of get_full_response()
        Since the data is in a known format (ascii), we can decode it directly
        It is also quicker as the decode can be applied to the whole message instead of doing it per byte
        '''
        self.can_bus.flush_tx_buffer()
        self.send("data", msg + "\n")
        cli_tag = cli_tag.encode("ascii")
        self.fast_single_response(filters)

        prev_msg: bytearray = bytearray()
        more_data = True
        while more_data:
            recv_msg = self.fast_single_response(filters)
            # accounts for the cli_tag being split across 2 messages
            if recv_msg is not None and cli_tag in prev_msg + recv_msg:
                more_data = False
                break
            # allows to build message line by line
            yield recv_msg
            if (reply_reader is not None or prompt_reader is not None) and (
                    b"Reply ready/skip/abort + Enter." in recv_msg
                    or b"reply read/pass/fail/skip/abort, then Enter." in recv_msg):
                reply = prompt_reader(bytes(recv_msg)) if prompt_reader else reply_reader()
                self.send("data", reply + "\n")
            prev_msg = recv_msg

    def get_single_response(self):
        self.can_bus.flush_tx_buffer()

        msg_size = int(self.recv())
        recv_msg = ""
        i = 0
        while i < msg_size:
            tmp = int(self.recv())
            recv_msg += chr(tmp & 0xFF)
            recv_msg += chr((tmp >> 8) & 0xFF)
            recv_msg += chr((tmp >> 16) & 0xFF)
            recv_msg += chr((tmp >> 24) & 0xFF)
            i += 4

        # filter unwanted messages
        if "PEC Rate" in recv_msg:
            return ""
        return recv_msg

    def get_full_response(self, msg, cli_tag):
        # capture newline at start of every msg
        self.send("data", msg + "\n")
        self.get_single_response()

        prev_msg = ""
        # recieve actual data
        more_data = True
        while more_data:
            recv_msg = self.get_single_response().lstrip("\n")
            # accounts for the cli_tag beign split across 2 messages
            if cli_tag in (prev_msg+recv_msg):
                more_data = False
                break
            # allows to build message line by line
            yield recv_msg
            prev_msg = recv_msg


def read_bench_reply(timeout=100):
    # Abort before the firmware's 120-second reply timeout. Read stdin without
    # leaving an input thread behind if the operator disconnects.
    print("Fixture reply> ", end="", flush=True)
    try:
        readable, _, _ = select.select([sys.stdin], [], [], timeout)
        if readable:
            return sys.stdin.readline().strip() or "abort"
    except KeyboardInterrupt:
        pass
    print("\nSending abort to the VCU.")
    return "abort"


def terminal(can_bus, fixture=None):
    usage = "Usage:\n"\
        " board> <command>\n"\
        "   - send RTOS command to board. Board must be selected prior\n"\
        "   Example: vcu> get_brake\n\n"\
        " board> <board[vcu|bmu|pdu|dcu|wsbfl|wsbfr|wsbrl|wsbrr|filters]>\n"\
        "   - switch to new board. > is required after board name\n"\
        "   - filters is a special board that is used to define exclusion filters\n"\
        "   Example: vcu> bmu>\n\n"\
        " filters> <command[--a|--d|--list]> <Optional[filter value]>\n"\
        "   - if a exclusion value is found in the line, line is omitted from message\n"\
        "   --a [filter value]: add the filter value to the exclusion list\n"\
        "   --d [filter value]: delete the filter value to the exclusion list\n"\
        "   --list: list the values in the exclusion list\n"\
        "   Example: filters>--a PEC Error Rate\n\n"\
        " [Ctrl + C] to exit\n\n"\

    print(f"\n{bcolors.OKCYAN}{usage}{bcolors.ENDC}\n")

    board_info = {
        "vcu": {
            "id": 0x01,
            "cli_tag": "vcu_F7 >"
        },
        "bmu": {
            "id": 0x02,
            "cli_tag": "bmu >"
        },
        "pdu": {
            "id": 0x04,
            "cli_tag": "pdu >"
        },
        "dcu": {
            "id": 0x08,
            "cli_tag": "dcu >"
        },
        "wsbfl": {
            "id": 0x10,
            "cli_tag": "wsbfl >"
        },
        "wsbfr": {
            "id": 0x20,
            "cli_tag": "wsbfr >"
        },
        "wsbrl": {
            "id": 0x40,
            "cli_tag": "wsbrl >"
        },
        "wsbrr": {
            "id": 0x80,
            "cli_tag": "wsbrr >"
        },
        "filters": {
            "filter_list": []
        },
    }
    board = "Choose a board"

    while True:
        msg = input(f"{bcolors.OKBLUE}{bcolors.BOLD}{board}> {bcolors.ENDC}")

        # check if msg is switching board,
        # reqd format: [board]>
        if msg[:-1] in board_info and msg[-1] == ">":
            board = msg[:-1]
            if board != "filters":
                board_id = board_info[board]["id"]
                # send the config message
                can_bus.send("config", board_id)
            continue
        elif board == "filters":
            if msg.startswith("--a "):
                msg = msg.replace("--a ", "")
                board_info["filters"]["filter_list"].append(
                    msg.strip().encode("ascii"))
            elif msg.startswith("--d "):
                msg = msg.replace("--d ", "")
                board_info["filters"]["filter_list"].remove(
                    msg.strip().encode("ascii"))
            elif msg.startswith("--list"):
                print(board_info["filters"]["filter_list"])
            else:
                print("Command not recognized")
        else:
            response = bytearray()
            bench_run = board == "vcu" and msg.split()[:2] == ["vcuTest", "run"]
            try:
                filters = [] if bench_run else board_info["filters"]["filter_list"]
                reply_reader = read_bench_reply if bench_run else None
                prompt_reader = None
                if bench_run and fixture is not None:
                    fixture.start()
                    def prompt_reader(prompt):
                        try:
                            return fixture.reply(prompt, lambda: read_bench_reply(timeout=30))
                        except (Exception, KeyboardInterrupt) as error:
                            print(f"HIL STOP: {error}; aborting VCU run.", flush=True)
                            return "abort"
                for line in can_bus.fast_full_response(msg, board_info[board]["cli_tag"], filters,
                                                      reply_reader=reply_reader,
                                                      prompt_reader=prompt_reader):
                    response.extend(line)
                    if bench_run:
                        print(line.rstrip(b"\x00").decode("ascii", errors="ignore"), end="", flush=True)
                if not bench_run:
                    print(response.decode("ascii", errors="ignore"))
            except (Exception, KeyboardInterrupt):
                if bench_run:
                    try:
                        can_bus.send("data", "abort\n")
                    except Exception:
                        print("VCU abort could not be delivered; wait for its reply timeout.")
                print(f"\n{bcolors.FAIL}{traceback.format_exc()}{bcolors.ENDC}\n")
                print(f"{bcolors.WARNING}Last Response:{bcolors.ENDC}\n")
                print(
                    f"\n{bcolors.FAIL}{response.decode('ascii', errors='ignore')}{bcolors.ENDC}\n")
            finally:
                if bench_run and fixture is not None:
                    try:
                        fixture.finish()
                    except Exception as error:
                        print(f"HIL RESTORE FAILED: {error}. Run is not validated.", flush=True)


def main():
    parser = argparse.ArgumentParser(description="Vehicle board CAN console with optional STM32 HIL control")
    parser.add_argument("--channel", default="can1", help="Vehicle SocketCAN interface")
    parser.add_argument("--hil-port", help="HIL UART4 serial adapter device")
    parser.add_argument("--hil-profile", help="Verified HIL_2027 fixture JSON profile")
    args = parser.parse_args()
    if bool(args.hil_port) != bool(args.hil_profile):
        parser.error("--hil-port and --hil-profile are required together")
    fixture = None
    port = None
    if args.hil_port:
        try:
            from hil_fixture import HilFixture, load_profile
            profile = load_profile(args.hil_profile)  # Validate before any board I/O.
            import serial
            port = serial.Serial(args.hil_port, 115200, timeout=0.1, write_timeout=0.5)
            fixture = HilFixture(port, profile)
        except (ImportError, ValueError, OSError) as error:
            parser.error(str(error))
    can_bus = None
    try:
        can_bus = UartOverCanInterface(channel=args.channel)
        terminal(can_bus, fixture)
    finally:
        if port is not None:
            port.close()
        if can_bus is not None:
            can_bus.can_bus.shutdown()


if __name__ == "__main__":
    main()
