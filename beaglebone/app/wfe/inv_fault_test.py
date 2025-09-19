
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


sigs = [
    cantools.db.can.signal.Signal('DTC_CODE', 0, 8),
    cantools.db.can.signal.Signal('DTC_Severity', 8, 8),
    cantools.db.can.signal.Signal('DTC_Data', 16, 32)
]
msg = cantools.db.can.message.Message(0xff03, 'PDU_DTC',6, signals=sigs, is_extended_frame=True )

db._add_message(msg)
db_msg = db.get_message_by_name("PDU_DTC")

ARB_ID = db_msg.frame_id
for fault in INV_FAULT_CODES_DESC:
    run_fault = fault & 0xffffffff
    post_fault = fault >> 32
    data = {
            'DTC_CODE': 60 if post_fault else 61,
            'DTC_Severity': 4,
            'DTC_Data': post_fault if post_fault else run_fault,
        }
    print(hex(fault), INV_FAULT_CODES_DESC[fault])
    msg_data = db.encode_message(ARB_ID, data, strict=False)

    msg = can.Message(arbitration_id=ARB_ID, data=msg_data)

    can_bus.send(msg)
