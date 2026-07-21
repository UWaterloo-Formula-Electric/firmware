"""Phase 1: Verify CAN bus traffic on a SocketCAN interface."""

import can

# Change to "can0" when running on the actual car.
CHANNEL = "vcan0"
INTERFACE = "socketcan"


def main() -> None:
    bus = can.interface.Bus(channel=CHANNEL, interface=INTERFACE)

    print(f"Listening on {CHANNEL}... (Ctrl+C to stop)\n")
    print(f"{'ID':<12} {'Data':<20} Timestamp")
    print("-" * 52)

    try:
        while True:
            msg = bus.recv()
            can_id = f"0x{msg.arbitration_id:03X}"
            data = msg.data.hex()
            timestamp = f"{msg.timestamp:.6f}"
            print(f"{can_id:<12} {data:<20} {timestamp}")
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        bus.shutdown()


if __name__ == "__main__":
    main()
