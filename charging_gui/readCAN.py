import cantools
from can.interfaces.vector import VectorBus
import cantools.database

# filters to read only the battery voltage and temperature messages
filters = [
    # temperature
    {"can_id": 0x98C00401, "can_mask": 0x1FFFFFFF, "extended": True}, 
    # cell voltage
    {"can_id": 0x98800401, "can_mask": 0x1FFFFFFF, "extended": True}
]

can_bus = VectorBus(channel=1, can_filters=filters)
db = cantools.database.load("common/Data/2024CAR.dbc")

BATTERYSTATUS_TEMPERATURE_ID = db.get_message_by_name('BMU_ChannelTemp').frame_id
BATTERYSTATUS_VOLTAGE_ID = db.get_message_by_name('BMU_CellVoltage').frame_id


def process_can_message(msg):
    """
    Process a CAN message and extract the battery voltage and temperature.
    """
    print("reading can messages...")
    while True:
        message = can_bus.recv(timeout=0.1)
        if message is None:
            continue
        print(message)

        try:
            if msg.arbitration_id == BATTERYSTATUS_TEMPERATURE_ID:
                decoded = db.decode_message(BATTERYSTATUS_TEMPERATURE_ID, msg.data)
                # Collect all temperature channel values into a list
                temperature_channels = []
                for i in range(1, 190):
                    channel_name = f"TempChannel{i:02d}" # TempChannel01 to TempChannel189
                    temperature_channels.append(decoded[channel_name])
                print(f"Battery Temperature [01]: {temperature_channels[0]}")
            
            elif msg.arbitration_id == BATTERYSTATUS_VOLTAGE_ID:
                decoded = db.decode_message(BATTERYSTATUS_VOLTAGE_ID, msg.data)
                voltage_channels = []
                # Extract the voltage value
                for i in range(1, 141):
                    channel_name = f"VoltageCell{i:02d}" # VoltageCell01 to VoltageCell140
                    voltage_channels.append(decoded[channel_name])
                print(f"Battery Voltage [01]: {voltage_channels[0]}")
        except Exception as e:
            print(f"Error decoding message: {e}")
            continue