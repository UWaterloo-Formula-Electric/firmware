import cantools
import can
import time

db = cantools.database.load_file("common/Data/2024CAR.dbc")
can_bus = can.interface.Bus('vcan0', bustype='socketcan')
message = db.get_message_by_name('Read_IMD_Error_Threshold')
signal_dict = {'IMD_Error_Threshold': 100}
data = message.encode(signal_dict)
msg = can.Message(arbitration_id=message.frame_id, data=data, is_extended_id=message.is_extended_frame)

can_bus.send(msg)
time.sleep(0.5)

message = can_bus.recv()
decoded = message.decode(message.data)
print(decoded)







