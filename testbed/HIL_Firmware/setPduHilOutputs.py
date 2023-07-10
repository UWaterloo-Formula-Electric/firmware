import os
import can
import cantools
import math
import time

MAX_POT_RESISTANCE_OHM = 199219
MIN_POT_RESISTANCE_OHM = 60

bus = can.Bus(bustype='vector', app_name='CANalyzer', channel=0, bitrate=500000)
hil_dbc = cantools.database.load_file(r'testbed\HIL_Firmware\common\data\HIL.dbc')

pdu_hil = cantools.tester.Tester('PDU_HIL', hil_dbc, bus)
pdu_hil.start()

#resistance in Ohm's
def setPotResistance(resistance):
    resistance = math.trunc(resistance)
    if(resistance > MAX_POT_RESISTANCE_OHM):
        resistance = MAX_POT_RESISTANCE_OHM
    elif(resistance < MIN_POT_RESISTANCE_OHM):
        resistance = MIN_POT_RESISTANCE_OHM
    try:
        pdu_hil.send("Batt_thermistor", {"Batt_thermistor": resistance})
        print(f"message sent on {bus.channel_info}")
    except:
        print("message NOT sent")
        return False
    try:
        pdu_hil.expect('PDU_message_status', None,0.01)
        print("message received")
        return True
    except can.CanError:
        print("status message not received!")
        return False
    