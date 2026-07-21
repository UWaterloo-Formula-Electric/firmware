"""Phase 3: Log CAN messages to a CSV file."""

import can
import csv
import os
from datetime import datetime

# Change to "can0" when running on the actual car.
CHANNEL = "vcan0"
INTERFACE = "socketcan"
BUFFER_SIZE = 100
LOG_DIR = "logs"


def get_log_filepath() -> str:
    date_str = datetime.now().strftime("%Y_%m_%d")
    session_dir = os.path.join(LOG_DIR, f"session_{date_str}")
    os.makedirs(session_dir, exist_ok=True)

    # Auto-increment session number
    existing = [f for f in os.listdir(session_dir) if f.startswith("session") and f.endswith(".csv")]
    session_num = len(existing) + 1

    return os.path.join(session_dir, f"session{session_num}.csv")


def main() -> None:
    bus = can.interface.Bus(channel=CHANNEL, interface=INTERFACE)
    filename = get_log_filepath()

    print(f"Listening on {CHANNEL}...")
    print(f"Logging to {filename}")
    print("(Ctrl+C to stop)\n")

    buffer = []

    with open(filename, "w", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(["timestamp", "id", "dlc", "extended", "data"])

        try:
            while True:
                msg = bus.recv()

                row = [
                    f"{msg.timestamp:.6f}",
                    f"0x{msg.arbitration_id:03X}",
                    msg.dlc,
                    msg.is_extended_id,
                    msg.data.hex()
                ]

                buffer.append(row)
                print(f"ID: {row[1]:<12} DLC: {row[2]:<4} Data: {row[4]}")

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