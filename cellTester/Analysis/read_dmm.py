# make sure pyvisa is setup
import csv
import os
import pyvisa as visa
from serial.tools import list_ports
from serial import Serial
import time
from datetime import datetime, timedelta
import sys
sys.path.append('../')
from GUI.CachedData import CachedData
from GUI.CellData import CellData
from GUI.constants import *
# load visa lib and open connection
rm = visa.ResourceManager()

# In this example com port 7 is assigned to the USB-to-IR adapter
# Address 'ASRL7::INSTR' is equivalent to com port 7.
# Find port and connect
dmm_port_num = None
for port, desc, hwid in list_ports.comports():
    if "Prolific" in desc:
        dmm_port_num =  int(port.removeprefix("COM"))
        break

assert dmm_port_num is not None, "Could not find DMM. Check connection and try again."

ct_port = None
# Find port and connect
for port, desc, hwid in list_ports.comports():
    if "STLink" in desc:
        ct_port = Serial(port, 230400, timeout=None)
        break

assert ct_port is not None, "No STLink found"


def read_dmm(command: str) -> str:
    # Get the value of the primary display
    dmm.write(command)
    dmm_data = dmm.read().strip()
    return dmm_data

def write_dmm(command: str):
    dmm.write(command)


def identify_data(line: bytes):
    data = line.decode("ascii").strip().split(", ")
    if len(data) == 1:
        return (RX_DATA_TYPE_CELL_TESTER_STATUS, data)
    else:
        return (RX_DATA_TYPE_LOGGING_DATA, data)

cachedData = CachedData(ct_port)
file_name = datetime.now().strftime("%b-%d_%H-%M-%S") + "temp_cal.csv"
cell_logfile_csv = open(file_name, "w", newline='')
cell_logfile_writer = csv.writer(cell_logfile_csv)
header = ["Timestamp [ms]", "True Temp [C]", "Voltage1 [mV]", "Voltage2 [mV]", "Temp1 [C]", "Temp2 [C]"]
cell_logfile_writer.writerow(header)



with rm.open_resource(f'ASRL{dmm_port_num}::INSTR') as dmm:
    # get U1282A Identification
    print(read_dmm("*IDN?"))
    mode = read_dmm("CONF?")
    assert mode == "\"TEMP:K CEL\"", f"DMM {mode=}. Set to temperature mode"
    # should be in temperature mode now
    if not os.path.exists(LOG_DIR):
        os.mkdir(LOG_DIR)
    first_ts = None
    while True:
        line = cachedData.get_unread_lines()[-1]
        data_type, data = identify_data(line)
        if data_type == RX_DATA_TYPE_CELL_TESTER_STATUS:
            continue
        # Get the value of the primary display  
        try:
            if first_ts is None:
                first_ts = datetime.now().timestamp()
            timestamp = (datetime.now().timestamp() - first_ts) * 1000
            true_temp = float(read_dmm("READ?"))
            measured_v1 = float(data[1])
            measured_v2 = float(data[2])
            measured_temp1 = float(data[3])
            measured_temp2 = float(data[4])
            csv_row = [timestamp, true_temp, measured_v1, measured_v2, measured_temp1, measured_temp2]
            print(csv_row)
            cell_logfile_writer.writerow(csv_row)
            
        except (IndexError, ValueError):
            pass

