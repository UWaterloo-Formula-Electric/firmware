"""Phase 2: Read and print CAN messages with full frame details."""

import can

# Change to "can0" when running on the actual car.
CHANNEL = "vcan0"
INTERFACE = "socketcan"


def main() -> None:
    bus = can.interface.Bus(channel=CHANNEL, interface=INTERFACE)

    print(f"Listening on {CHANNEL}... (Ctrl+C to stop)\n")
    print(f"{'ID':<12} {'DLC':<6} {'Extended':<10} {'Data':<20} Timestamp")
    print("-" * 70)

    try:
        while True:
            msg = bus.recv()
            can_id = f"0x{msg.arbitration_id:03X}"
            dlc = msg.dlc
            extended = msg.is_extended_id
            data = msg.data.hex()
            timestamp = f"{msg.timestamp:.6f}"
            print(f"{can_id:<12} {dlc:<6} {str(extended):<10} {data:<20} {timestamp}")
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        bus.shutdown()


if __name__ == "__main__":
    main()