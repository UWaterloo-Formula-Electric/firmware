"""Phase 4: Decode CAN messages using the DBC file."""

import can
import cantools
import csv
import os
from datetime import datetime

# Change to "can0" when running on the actual car.
CHANNEL = "vcan0"
INTERFACE = "socketcan"
DBC_FILE = "2024CAR.dbc"
BUFFER_SIZE = 100
LOG_DIR = "logs"


def get_log_filename() -> str:
    os.makedirs(LOG_DIR, exist_ok=True)
    timestamp = datetime.now().strftime("%Y_%m_%d_%H_%M_%S")
    return os.path.join(LOG_DIR, f"can_decoded_{timestamp}.csv")


def main() -> None:
    db = cantools.database.load_file(DBC_FILE)
    bus = can.interface.Bus(channel=CHANNEL, interface=INTERFACE)
    filename = get_log_filename()

    print(f"Loaded DBC: {DBC_FILE}")
    print(f"Listening on {CHANNEL}...")
    print(f"Logging to {filename}")
    print("(Ctrl+C to stop)\n")

    buffer = []

    try:
        with open(filename, "w", newline="") as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(["timestamp", "id", "message_name", "signal_name", "value", "unit"])

            while True:
                msg = bus.recv()

                try:
                    db_msg = db.get_message_by_frame_id(msg.arbitration_id)
                    decoded = db_msg.decode(msg.data)

                    for signal_name, value in decoded.items():
                        signal = db_msg.get_signal_by_name(signal_name)
                        unit = signal.unit if signal.unit else ""

                        row = [
                            f"{msg.timestamp:.6f}",
                            f"0x{msg.arbitration_id:08X}",
                            db_msg.name,
                            signal_name,
                            value,
                            unit
                        ]
                        buffer.append(row)
                        print(f"{db_msg.name:<35} {signal_name:<40} {value} {unit}")

                except KeyError:
                    # Frame ID not in DBC - log raw and move on
                    row = [
                        f"{msg.timestamp:.6f}",
                        f"0x{msg.arbitration_id:08X}",
                        "UNKNOWN",
                        "RAW",
                        msg.data.hex(),
                        ""
                    ]
                    buffer.append(row)

                if len(buffer) >= BUFFER_SIZE:
                    writer.writerows(buffer)
                    csvfile.flush()
                    buffer.clear()

    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        if buffer:
            writer.writerows(buffer)
            csvfile.flush()
        bus.shutdown()
        print(f"Log saved to {filename}")


if __name__ == "__main__":
    main()