
import csv
import getpass
from pathlib import Path
import random
import can
import cantools
from pprint import pprint
from dashboard.constants import INV_FAULT_CODES_DESC

if getpass.getuser() == 'vagrant':
    home_dir = Path("/home/vagrant/shared")
    CANBUS = 'vcan0'
else:
    home_dir = Path(__file__).resolve().parents[4]
db = cantools.db.load_file(home_dir / 'firmware/common/Data/2024CAR.dbc')
can_bus = can.interface.Bus(channel=CANBUS, bustype='socketcan')
db_msg = db.get_message_by_name("MC_Fault_Codes")
ARB_ID = db_msg.frame_id
for fault in INV_FAULT_CODES_DESC:
    data = {
        'INV_Run_Fault_Hi': fault >> 47 & 0xFFFF,
        'INV_Run_Fault_Lo': fault >> 31 & 0xFFFF,
        'INV_Post_Fault_Hi': fault >> 15 & 0xFFFF,
        'INV_Post_Fault_Lo': fault & 0xFFFF,
    }
    print(hex(fault), INV_FAULT_CODES_DESC[fault])
    msg_data = db.encode_message(ARB_ID, data)

    msg = can.Message(arbitration_id=ARB_ID, data=msg_data)

    can_bus.send(msg)