
import os
from pathlib import Path
import can
import cantools

if os.getlogin() == 'vagrant':
    CANBUS = 'vcan0'
    home_dir = Path("/home/vagrant/shared")
else:
    CANBUS = 'can1'
    home_dir = Path("/home/debian")
db = cantools.db.load_file(home_dir / 'firmware/common/Data/2024CAR.dbc')
can_bus = can.interface.Bus(channel=CANBUS, bustype='socketcan')

db_msg = db.get_message_by_name('BMU_batteryStatusHV')
ARB_ID = db_msg.frame_id
data = {}
for signal in db_msg.signals:
    data[signal.name] = 0

data['StateBatteryChargeHV'] = 54.6
data['TempCellMax'] = 32.5
# data['ButtonScreenNavLeftEnabled'] = 1

msg_data = db.encode_message(ARB_ID, data)

msg = can.Message(arbitration_id=ARB_ID, data=msg_data)

can_bus.send(msg)

