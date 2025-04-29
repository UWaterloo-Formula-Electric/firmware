import csv
import getpass
from pathlib import Path
import random
import can
import cantools
from pprint import pprint
from dashboard.constants import INTERLOCK_FAULT_CODES_DESC
import time

if getpass.getuser() == 'vagrant':
    home_dir = Path("/home/vagrant/shared")
    CANBUS = 'vcan0'
else:
    home_dir = Path(__file__).resolve().parents[4]

db = cantools.db.load_file(home_dir / 'firmware/common/Data/2024CAR.dbc')

# can_bus = can.interface.Bus(channel=CANBUS, bustype='socketcan')
can_bus = can.Bus(interface='virtualcan', channel='localhost:18881')


msg = db.get_message_by_name('BMU_Interlock_Loop_Status')

while 1:
    for i in INTERLOCK_FAULT_CODES_DESC.keys():
        msg_data = db.encode_message(msg.frame_id, {'BMU_checkFailed': i}, strict=False)
        can_msg = can.Message(arbitration_id=msg.frame_id, data=msg_data)
        can_bus.send(can_msg)
        time.sleep(0.3)
# while 1: 1
# sigs = [
#     cantools.db.can.signal.Signal('DTC_CODE', 0, 8),
#     cantools.db.can.signal.Signal('DTC_Severity', 8, 8),
#     cantools.db.can.signal.Signal('DTC_Data', 16, 32)
# ]
# msg = cantools.db.can.message.Message(0xff03, 'PDU_DTC',6, signals=sigs, is_extended_frame=True )
