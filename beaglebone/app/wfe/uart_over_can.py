import time
import can
import cantools
from util import default_dbc_path, bcolors


class UartOverCanInterface():

    DEFAULT_DBC = default_dbc_path()

    def __init__(self, interface="can1", dbc=DEFAULT_DBC):
        self.db = cantools.database.load_file(dbc)
        self.timeout = 5
        self.interface = interface

        print(f"Initializing CAN monitor on {self.interface}")
        self.can_bus = can.interface.Bus(self.interface, bustype="socketcan", can_filters=[
            {"can_id": 0x40602FF, "can_mask": 0xFFFFFFF, "extended": True}])

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
            raise can.CanError(f"Failed to receive a CAN message on {self.interface} for {self.timeout} "
                               "seconds")
        # Dictionary of decoded values and their signals
        decoded_data = self.db.decode_message(
            msg.arbitration_id,
            msg.data
        )

        return decoded_data["UartOverCanRX"]

    def get_single_response(self, msg):
        self.can_bus.flush_tx_buffer()
        self.send("data", msg)

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
        self.get_single_response(msg + "\n")
        # recieve actual data
        more_data = True
        while more_data:
            recv_msg = self.get_single_response("")
            if cli_tag and cli_tag in recv_msg:
                more_data = False
                break
            # allows to build message line by line
            yield recv_msg


def main():
    can_bus = UartOverCanInterface()

    usage = "Usage:\n"\
        "  board> <command>                 - send RTOS command to board. Board must be selected prior\n"\
        "                                   Example: vcu> get_brake\n\n"\
        "  board> <board[vcu|bmu|pdu|dcu]>  - switch to new board. > is required after board name\n"\
        "                                   Example: vcu> bmu>\n\n"

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
            "cli_tag": None
        },
        "wsbfl": {
            "id": 0x10,
            "cli_tag": None
        },
        "wsbfr": {
            "id": 0x20,
            "cli_tag": None
        },
        "wsbrl": {
            "id": 0x40,
            "cli_tag": None
        },
        "wsbrr": {
            "id": 0x80,
            "cli_tag": None
        },
    }
    board = "Choose a board"
    while True:
        msg = input(board + "> ")

        # check if msg is switching board,
        # reqd format: [board]>
        # board must be 3 letters long
        if len(msg) == 4 and msg[3] == ">":
            board = msg[:-1]
            board_id = board_info[board]["id"]
            # send the config message
            can_bus.send("config", board_id)
            continue
        else:
            response = ""
            try:
                for line in can_bus.get_full_response(msg, board_info[board]["cli_tag"]):
                    response += line
                    print(line)
            except Exception as e:
                print(f"\n{bcolors.FAIL}{e}{bcolors.ENDC}\n")
                print(f"{bcolors.WARNING}Last Response:{bcolors.ENDC}\n")
                print(f"\n{bcolors.FAIL}{response}{bcolors.ENDC}\n")


if __name__ == "__main__":
    main()
