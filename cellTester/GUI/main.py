import math
import csv
from io import StringIO

from serial.tools import list_ports
from serial import Serial
import pandas as pd
import os
import datetime

import PySimpleGUI as sg

from CellData import CellData
from constants import *
from CachedData import CachedData




def send_current_target(current: float):
    ct_port.write(f"{current:.3}\n".encode("ascii"))


def identify_data(line: bytes):
    data = line.decode("ascii").strip().split(", ")
    if len(data) == 1:
        return (RX_DATA_TYPE_CELL_TESTER_STATUS, data)
    else:
        return (RX_DATA_TYPE_LOGGING_DATA, data)


def get_characterization(data):
    # Expected format: "timestamp, voltage, current, temperature"
    try:
        ret = int(data[0]), float(data[1]), float(data[2]), float(data[3])
    except:
        print(data)
        raise Exception("invalid data printed from cell tester") 
    return ret


# Find port and connect
ct_port = None
for port, desc, hwid in list_ports.comports():
    if "STLink" in desc:
        ct_port = Serial(port, 230400, timeout=None)
        break

assert ct_port is not None, "No STLink found"

cell_df = pd.DataFrame(columns=["Time [ms]", "IShunt [A]", "V1 [V]", "V2 [V]"])
logDir = "../CellData/"
current_date = datetime.datetime.now()
cellLogfile = current_date.strftime("%b-%d_%H-%M-%S") + "_cell_{cellNum}" + ".csv"
isExist = os.path.exists(logDir)
if not isExist:
    os.mkdir(logDir)

cell_number = 1
isTestingCell = False
cachedData = CachedData(ct_port)

sg.theme('DarkGrey14')
# All the stuff inside your window.

layout = [
    [sg.Frame('Meters', [
        [
            sg.Frame('Voltage [V]', [
                [sg.Text('0.0', key='cell_V', size=10, justification='center', font=('Consolas', 30))]]),
            sg.Frame('Current [A]', [
                [sg.Text('0.0', key='cell_I', size=10, justification='center', font=('Consolas', 30))]])
        ],
        [
            sg.Frame('Temperature [°C]', [
                [sg.Text('0.0', key='cell_T', size=10, justification='center', font=('Consolas', 30))]]),
            sg.Frame('Derived Internal Resitance [Ω]', [
                [sg.Text('0.0', key='cell_R', size=10, justification='center', font=('Consolas', 30))]])
        ]])
    ],
    [
        sg.Frame('Is Logging Cell Data', [
            [sg.StatusBar('False', key='isLogging', justification='center', size=12, font=('Consolas'))]]),
        sg.Frame('Set Current Target [A]', [
            [sg.InputText('0.0', key='set_current_target', size=10, justification='center'),
             sg.Button('Update', size=10)]
        ])
    ],
    [
        sg.Frame("Cell Test Control", [
            [sg.Text('File Name:')], 
            [sg.InputText(cellLogfile.format(cellNum=cell_number), key='file_name', size=43)],
            [sg.Button('Start Cell Test', size=19),
             sg.Button('Stop Cell Test', size=19)]
        ])
    ],   
]

# Create the Window
window = sg.Window('Cell Tester', layout, size=(500, 375))
# Event Loop to process "events" and get the "values" of the inputs
while True:
    event, values = window.read(timeout=10)  # type: ignore
    if event == sg.WIN_CLOSED:
        break
    elif event == 'Start Cell Test':
        isExist = os.path.exists(logDir)
        if not isExist:
            os.mkdir(logDir)

        file_name = values['file_name']
        cell_df.to_csv(f'../CellData/{file_name}',
                       index=False, lineterminator='\n')
        cell_df.drop(cell_df.index, inplace=True)  # reset dataframe

        cell_logfile_csv = open(logDir + cellLogfile.format(cellNum=cell_number), "w", newline='')
        cell_logfile_writer = csv.writer(cell_logfile_csv)
        cell_logfile_writer.writerow(cell_df.columns)
        cell_logfile_csv.flush()
        
        ct_port.write("start\r\n".encode("ascii"))
    elif event == 'Stop Cell Test':
        # Send stop command to cell tester board. Wait for ack from cell tester board before updating GUI logging indicator
        ct_port.write("stop\r\n".encode("ascii"))
    elif event == 'Update':
        # Update Current Target
        send_current_target(float(values['set_current_target']))
    else:
        # Get data
        for line in cachedData.get_unread_lines():
            (data_type, data) = identify_data(line)
            if data_type == RX_DATA_TYPE_CELL_TESTER_STATUS:
                print(data)
                if (data[0] == "start"):
                    # Cell tester ack'ed the start cell test, start logging
                    window['isLogging'].update(value='True')

                    # A cell test is in progress
                    isTestingCell = True
                elif (data[0] == "stop"):
                    # Cell tester signified the end of cell test, stop logging
                    window['isLogging'].update(value='False')
                    
                    # Set up next file name
                    cell_number += 1
                    window['file_name'].update(value=cellLogfile.format(cellNum=cell_number))

                    # A cell test is not in progress
                    isTestingCell = False
                else:
                    print(f"unknown command {data[0]}")
            elif data_type == RX_DATA_TYPE_LOGGING_DATA:
                try:
                    cell_data = CellData(*get_characterization(data))

                    # Update values
                    if cell_data.voltage > 0.1:
                        window['cell_V'].update(value=f"{cell_data.voltage:.3f}")
                    if cell_data.current > 0.01:
                        window['cell_I'].update(value=f"{cell_data.current:.3f}")
                    if cell_data.temperature > 1:
                        window['cell_T'].update(value=f"{cell_data.temperature:.2f}")
                
                    # Open Circuit Voltage is whenever the current is less than VOLTAGE_OC_CURRENT_THRESHOLD
                    cell_data.voltage_open_circuit = cell_df.loc[cell_df["IShunt [A]"] <=
                                                                    VOLTAGE_OC_CURRENT_LIMIT]["V1 [V]"].mean()
                    window['cell_R'].update(value=cell_data.resistance)
                except Exception as e:
                    print(e)
                    continue

                if isTestingCell:
                    try:
                        # Log data
                        cell_logfile_writer.writerow(cell_data.formatted_data())
                        cell_logfile_csv.flush()
                    except Exception as e:
                        print(e)
                        continue
            else:
                print("data type {} is unhandled".format(data_type))

window.close()
cell_logfile_csv.close()
