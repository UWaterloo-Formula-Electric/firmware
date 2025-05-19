import cantools
import can
import cantools.database

# filters to read only the battery voltage and temperature messages
filters = [
    # temperature
    {"can_id": 0x98C00401, "can_mask": 0x1FFFFFFF, "extended": True}, 
    # cell voltage
    {"can_id": 0x98800401, "can_mask": 0x1FFFFFFF, "extended": True}
]

can_bus = can.interface.Bus(channel="0", interface='vector', can_filters=filters)
db = cantools.database.load("common/Data/2024CAR.dbc")