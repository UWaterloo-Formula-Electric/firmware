import traceback
import re

import can.interfaces.socketcan
import can
import cantools
from util import default_dbc_path, bcolors


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

    def fast_full_response(self, msg, cli_tag, filters):
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


def main():
    can_bus = UartOverCanInterface()

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
            try:
                for line in can_bus.fast_full_response(msg, board_info[board]["cli_tag"], board_info["filters"]["filter_list"]):
                    response.extend(line)
                print(response.decode("ascii", errors="ignore"))
            except Exception:
                print(f"\n{bcolors.FAIL}{traceback.format_exc()}{bcolors.ENDC}\n")
                print(f"{bcolors.WARNING}Last Response:{bcolors.ENDC}\n")
                print(
                    f"\n{bcolors.FAIL}{response.decode('ascii', errors='ignore')}{bcolors.ENDC}\n")


if __name__ == "__main__":
    main()
