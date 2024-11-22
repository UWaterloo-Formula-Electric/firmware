import can
import struct
import time
import os

bus = can.Bus(channel='can0', bustype='socketcan')

def send_command(parameter_address, read=True):
    r_w_command = 0 if read else 1
    message_data = struct.pack('<HHB2x', parameter_address, parameter_address, r_w_command)
    arbitration_id = 0x0CFFC106
    msg = can.Message(arbitration_id=arbitration_id, data=message_data, is_extended_id=True)
    bus.send(msg)

def read_response():
    msg = bus.recv(timeout=1)
    if msg is None:
        return None
    if msg.arbitration_id != 0x0CFFC206:
        return None
    success = msg.data[2] == 1
    if not success:
        return None
    data = struct.unpack('<H', msg.data[4:6])[0]
    return data

with open("./common/Data/parameters.txt", "r") as file1:
    with open("./common/Data/response_data.txt", "a") as output_file:
        for line in file1:
            parameter_list = line.strip().split(" ")
            parameter_address = int(parameter_list[0], 16) 
            parameter_name = parameter_list[1]  

            send_command(parameter_address, read=True)
            time.sleep(0.1)  
            response_data = read_response()

            if response_data is not None:
                output_file.write(f"Parameter Name: {parameter_name}, Address: {parameter_address:#04x}, Response Data: {response_data:#04x}\n")
