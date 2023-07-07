import os
import can
import cantools
import math
import time
from pathlib import Path

MAX_VOLTAGE_V = 3.3
MAX_POT_RESISTANCE_OHM = 199219
MIN_POT_RESISTANCE_OHM = 60

bus = can.Bus(bustype='vector', app_name='CANalyzer', channel=0, bitrate=500000)
hil_dbc = cantools.database.load_file(r'testbed\HIL_Firmware\common\data\HIL.dbc')
vcu_hil = cantools.tester.Tester('VCU_HIL', hil_dbc, bus)
pdu_hil = cantools.tester.Tester('PDU_HIL', hil_dbc, bus)
vcu_hil.start()
pdu_hil.start()

#dac should match message name, voltage in V
def setDac(dac, voltage):
    if(voltage>MAX_VOLTAGE_V):
        voltage = MAX_VOLTAGE_V
    elif(voltage < 0):
        voltage = 0
    voltage= math.trunc(voltage*1000)
    try:
        vcu_hil.send(dac, {dac: voltage})
        print(f"message sent on {bus.channel_info}")
    except:
        print("message NOT sent")
        return False
    try:
        vcu_hil.expect('VCU_message_status', None,0.01)
        print("message received")
        return True
    except can.CanError:
        print("status message not received!")
        return False
    
#resistance in Ohm's
def setPotResistance(resistance):
    resistance = math.trunc(resistance)
    if(resistance > MAX_POT_RESISTANCE_OHM):
        resistance = MAX_POT_RESISTANCE_OHM
    elif(resistance < MIN_POT_RESISTANCE_OHM):
        resistance = MIN_POT_RESISTANCE_OHM
    try:
        vcu_hil.send('Batt_thermistor', {'Batt_thermistor': resistance})
        print(f"message sent on {bus.channel_info}")
    except:
        print("message NOT sent")
        return False
    try:
        vcu_hil.expect('PDU_message_status', None,0.01)
        print("message received")
        return True
    except can.CanError:
        print("status message not received!")
        return False