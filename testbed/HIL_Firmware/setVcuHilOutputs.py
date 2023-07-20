import os
import can
import cantools
import math
import time

MAX_VOLTAGE_V = 3.3

bus = can.Bus(bustype='vector', app_name='CANalyzer', channel=0, bitrate=500000)
hil_dbc = cantools.database.load_file(r'testbed\HIL_Firmware\common\data\HIL.dbc')
vcu_hil = cantools.tester.Tester('VCU_HIL', hil_dbc, bus)
vcu_hil.start()

#voltage in V
#TODO: test what happens on exceptions
def setThrottleA(voltage):
    if(voltage > MAX_VOLTAGE_V):
        voltage = MAX_VOLTAGE_V
    elif(voltage < 0):
        voltage = 0
    voltage= math.trunc(voltage*1000)
    try:
        vcu_hil.send("Throttle_position_A", {"Throttle_position_A": voltage})
        print(f"message sent on {bus.channel_info}")
    except can.CanError as error:
        print("message NOT sent "+ error)
        return False
    try:
        vcu_hil.expect('VCU_message_status', None, 0.01)
        print("message received")
        return True
    except can.CanError:
        print("status message not received!")
        return False
    
def setThrottleB(voltage):
    if(voltage > MAX_VOLTAGE_V):
        voltage = MAX_VOLTAGE_V
    elif(voltage < 0):
        voltage = 0
    voltage= math.trunc(voltage*1000)
    try:
        vcu_hil.send("Throttle_position_B", {"Throttle_position_B": voltage})
        print(f"message sent on {bus.channel_info}")
    except can.CanError as error:
        print("message NOT sent "+ error)
        return False
    try:
        vcu_hil.expect('VCU_message_status', None, 0.01)
        print("message received")
        return True
    except can.CanError:
        print("status message not received!")
        return False
    
def setSteering(voltage):
    if(voltage > MAX_VOLTAGE_V):
        voltage = MAX_VOLTAGE_V
    elif(voltage < 0):
        voltage = 0
    voltage= math.trunc(voltage*1000)
    try:
        vcu_hil.send("Steering_raw", {"Steering_raw": voltage})
        print(f"message sent on {bus.channel_info}")
    except can.CanError as error:
        print("message NOT sent "+ error)
        return False
    try:
        vcu_hil.expect('VCU_message_status', None, 0.01)
        print("message received")
        return True
    except can.CanError:
        print("status message not received!")
        return False
    
def setBrakePres(voltage):
    if(voltage > MAX_VOLTAGE_V):
        voltage = MAX_VOLTAGE_V
    elif(voltage < 0):
        voltage = 0
    voltage= math.trunc(voltage*1000)
    try:
        vcu_hil.send("Brake_pres_raw", {"Brake_pres_raw": voltage})
        print(f"message sent on {bus.channel_info}")
    except can.CanError as error:
        print("message NOT sent "+ error)
        return False
    try:
        vcu_hil.expect('VCU_message_status', None, 0.01)
        print("message received")
        return True
    except can.CanError:
        print("status message not received!")
        return False
    
def setBrakePosition(voltage):
    if(voltage > MAX_VOLTAGE_V):
        voltage = MAX_VOLTAGE_V
    elif(voltage < 0):
        voltage = 0
    voltage= math.trunc(voltage*1000)
    try:
        vcu_hil.send("Brake_position", {"Brake_position": voltage})
        print(f"message sent on {bus.channel_info}")
    except can.CanError as error:
        print("message NOT sent "+ error)
        return False
    try:
        vcu_hil.expect('VCU_message_status', None, 0.01)
        print("message received")
        return True
    except can.CanError:
        print("status message not received!")
        return False