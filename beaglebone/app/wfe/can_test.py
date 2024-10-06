
import csv
import getpass
from pathlib import Path
import random
import can
import cantools
from pprint import pprint

if getpass.getuser() == 'vagrant':
    home_dir = Path("/home/vagrant/shared")
    CANBUS = 'vcan0'
else:
    home_dir = Path(__file__).resolve().parents[4]
db = cantools.db.load_file(home_dir / 'firmware/common/Data/2024CAR.dbc')
can_bus = can.interface.Bus(channel=CANBUS, bustype='socketcan')

dtcs = {}
with open(home_dir / 'firmware/common/Data/DTC.csv', 'r') as file:
            reader = csv.reader(file)
            next(reader)  # Skip the header row
            for row in reader:
                dtc_code = int(row[0])
                origins = row[2].split(',')
                for origin in origins:
                    origin = origin.strip() + "_DTC"
                    if origin in ("VCU_BEAGLEBONE_DTC", "WSB_DTC"):
                         continue
                    dtcs[(dtc_code, origin)] = {
                        'name': row[1],
                        'origin': row[2],
                        'severity': row[3],
                        'subscribers': row[4],
                        'data': row[5],
                        'description': row[6],
                    }
for k, v in dtcs.items():
    dtc_code, dtc_name = k
    db_msg = db.get_message_by_name(dtc_name)
    ARB_ID = db_msg.frame_id

    desc_split = v['description'].split('{')
    reasons = {random.randint(0, 255)}
    if len(desc_split) > 1:
        reasons = eval("{" + desc_split[1])
    for reason_data in reasons:
        print(reason_data)
        data = {
            'DTC_CODE': dtc_code,
            'DTC_Severity': int(v['severity']),
            'DTC_Data': reason_data,
        }

        msg_data = db.encode_message(ARB_ID, data)

        msg = can.Message(arbitration_id=ARB_ID, data=msg_data)

        can_bus.send(msg)

# use the following to switch between dash pages on vm
# cansend vcan0 04011007#4000000000000000
# cansend vcan0 04011007#2000000000000000

