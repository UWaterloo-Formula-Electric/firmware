"""Phase 3: Log CAN messages to a CSV file."""

import can
import csv
import os
from datetime import datetime

# Change to "can0" when running on the actual car.
CHANNEL = "vcan0"
INTERFACE = "socketcan"

# Buffer size before flushing to disk
BUFFER_SIZE = 100

# Output directory
LOG_DIR = "logs"


def get_log_filename() -> str:
    os.makedirs(LOG_DIR, exist_ok=True)
    timestamp = datetime.now().strftime("%Y_%m_%d_%H_%M_%S")
    return os.path.join(LOG_DIR, f"can_log_{timestamp}.csv")


def main() -> None:
    bus = can.interface.Bus(channel=CHANNEL, interface=INTERFACE)
    filename = get_log_filename()

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
            # Flush any remaining frames in buffer
            if buffer:
                writer.writerows(buffer)
                csvfile.flush()

    bus.shutdown()
    print(f"Log saved to {filename}")


if __name__ == "__main__":
    main()