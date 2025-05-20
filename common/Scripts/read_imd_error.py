import cantools
import can
import time
from can.interfaces.vector import VectorBus

db = cantools.database.load_file("common/Data/2024CAR.dbc")
can_bus = VectorBus(channel=1)
message = db.get_message_by_name('Read_IMD_Error_Threshold')
target_id = message.frame_id
signal_dict = {'IMD_Index': 0x46}
data = message.encode(signal_dict)
msg = can.Message(arbitration_id=message.frame_id, data=data, is_extended_id=message.is_extended_frame)

can_bus.send(msg)

while True:
    message = can_bus.recv()
    if message.arbitration_id == target_id:
        decoded = db.decode_message(message.arbitration_id, message.data)
        print(decoded)







