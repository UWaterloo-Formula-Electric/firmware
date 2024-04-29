import cantools
import can


CANBUS = 'vcan0'

db = cantools.db.load_file('common/Data/2024CAR.dbc')
can_bus = can.interface.Bus(channel=CANBUS, bustype='socketcan')

# CAN arbitration ID constants
BATTERYSTATUSHV_ARB_ID = 2281967617
BMU_DTC_ARB_ID = 2147548929
MC_TEMP_ARB_ID = 2365563393
MC_TEMP_INV_ARB_ID = 2365562881

TEMPCOOLANT_L_ARB_ID = 2550137362
TEMPCOOLANT_R_ARB_ID = 2550137363

BMU_VBATT_ARB_ID = 2281769985
WHEELSPEED_ARB_ID = 2287471618
139987970
while True:
    message = can_bus.recv()
    message_data = db.decode_message(message.arbitration_id, message.data)
    arb_id = message.arbitration_id + 0x80000000
    if arb_id == WHEELSPEED_ARB_ID:
        print(message.arbitration_id, message_data)