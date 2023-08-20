import subprocess
from serial.tools import list_ports

hil_wican_port = None
for port, desc, hwid in sorted(list_ports.comports()):
    if "WiCAN Receiver" in desc:
        hil_wican_port = port
print(hil_wican_port)

assert subprocess.run(["slcand", "-l", "-s6", "-S 230400", hil_wican_port],
                      capture_output=True) == 0, f"{hil_wican_port} is already setup as slcan"
assert subprocess.run(["ip", "link", "set", "up", "slcan0"]) == 0
