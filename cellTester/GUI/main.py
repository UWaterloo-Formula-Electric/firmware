import math
import csv
import statistics

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


# Global Variables
cell_df = pd.DataFrame(columns=["Time [ms]", "IShunt [A]", "Cell Voltage [V]", "Cell Temp 1 [C]", "Cell Temp 2 [C]", "Internal Resitance [mOhm]"])
current_date = datetime.datetime.now()
cellLogfile = current_date.strftime("%b-%d_%H-%M-%S") + "_cell_{cellNum}" + ".csv"
ct_port = None
cell_logfile_csv = None
cell_data = CellData(timestamp_ms=0, current_A=0.0, voltage_V=0.0, temperature1_C=0.0, temperature2_C=0.0)
open_circuit_cell_voltages = [0]*NUM_SAMPLES_IN_OPEN_CIRCUIT_AVERAGE
open_circuit_cell_voltage_num_samples = 0

current_average_for_display = [0]*NUM_SAMPLES_IN_CURRENT_AVERAGE
current_average_for_display_sum = 0
current_average_for_display_index = 0

cell_number = 1
isTestingCell = False
error_message = "None"
wasFault = False

# The layout of the GUI
sg.theme('DarkGrey14')
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
            sg.Frame('Calculated Internal Resitance [mΩ]', [
                [sg.Text('0.0', key='cell_R', size=10, justification='center', font=('Consolas', 30))]])
        ]])
    ],
    [
        sg.Frame('Is Logging Cell Data', [
            [sg.StatusBar('False', key='is_logging', justification='center', size=12, font=('Consolas'))]]),
        sg.Frame('Set Current Target [A]', [
            [sg.InputText('0.0', key='set_current_target', size=10, justification='center'),
             sg.Button('Update', size=10)]
        ])
    ],
    [
        sg.Frame("Cell Test Control", [
            [sg.Text('File Name:', size=10),
             sg.Text(cellLogfile.format(cellNum=cell_number), key='file_name', justification='center', font=('Consolas'))], 
            [sg.Text('Cell Number:', size=10),
             sg.InputText("1", key='cell_number', size=10)],
            [sg.Button('Start Cell Test', size=19),
             sg.Button('Stop Cell Test', size=19)]
        ])
    ],
    [
        sg.Frame("Errors", [
            [sg.Text('None', key='errors', justification='center', font=('Consolas', 12))]
        ])
    ],
]
window = sg.Window('Cell Tester', layout, size=(500, 440))


def send_current_target(current: float):
    ct_port.write(f"{current:.3}\n".encode("ascii"))

def identify_data(line: bytes):
    data = line.decode("ascii").strip().split(", ")
    if len(data) == 1:
        return (RX_DATA_TYPE_CELL_TESTER_STATUS, data)
    else:
        return (RX_DATA_TYPE_LOGGING_DATA, data)

def get_characterization(data, open_circuit_cell_voltage):
    # Expected format: "timestamp, voltage, current, temperature"
    try:
        current = float(data[1])
        voltage = float(data[3]) - float(data[2])
        if current >= VOLTAGE_OC_CURRENT_LIMIT and open_circuit_cell_voltage != 0.0:
            resistance = (open_circuit_cell_voltage - voltage) / current
            ret = CellData(timestamp_ms=int(data[0]), current_A=current, voltage_V=voltage, temperature1_C=float(data[4]), temperature2_C=0.0, resistance_Ohm=resistance)
        else:
            ret = CellData(timestamp_ms=int(data[0]), current_A=current, voltage_V=voltage, temperature1_C=float(data[4]), temperature2_C=0.0)
    except:
        print(data)
        raise Exception("invalid data printed from cell tester") 
    return ret

def set_error_message(msg):
    if error_message != "None":
        error_message = msg


# Find port and connect
for port, desc, hwid in list_ports.comports():
    if "STLink" in desc:
        ct_port = Serial(port, 230400, timeout=None)
        break

assert ct_port is not None, "No STLink found"

cachedData = CachedData(ct_port)

# Event Loop to process "events" and get the "values" of the inputs
while True:
    error_message = "None"
    event, values = window.read(timeout=10)  # type: ignore
    if event == sg.WIN_CLOSED:
        break
    elif event == 'Start Cell Test':
        print("press")
        if wasFault:
            print("Can't start cell test due to fault")
            continue
        
        isExist = os.path.exists(LOG_DIR)
        if not isExist:
            os.mkdir(LOG_DIR)
        print("1")
        open_circuit_cell_voltages = [0]*NUM_SAMPLES_IN_OPEN_CIRCUIT_AVERAGE
        open_circuit_cell_voltage_num_samples = 0
        cell_number = float(values['cell_number'])
        print("2")
        if cell_number.is_integer():
            cell_number = int(cell_number)

        file_name = cellLogfile.format(cellNum=cell_number)
        window['file_name'].update(value=file_name)
        cell_df.to_csv(f'{LOG_DIR}{file_name}',
                       index=False, lineterminator='\n')
        cell_df.drop(cell_df.index, inplace=True)  # reset dataframe
        print("3")
        cell_logfile_csv = open(LOG_DIR + file_name, "w", newline='')
        cell_logfile_writer = csv.writer(cell_logfile_csv)
        cell_logfile_writer.writerow(cell_df.columns)
        cell_logfile_csv.flush()
        print("4")
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
                    window['is_logging'].update(value='True')

                    # A cell test is in progress
                    isTestingCell = True
                elif (data[0] == "stop"):
                    # Cell tester signified the end of cell test, stop logging
                    window['is_logging'].update(value='False')
                    
                    # Set up next file name
                    cell_number = math.floor(cell_number) + 1
                    window['file_name'].update(value=cellLogfile.format(cellNum=cell_number))
                    window['cell_number'].update(value=f"{cell_number}")
                    # A cell test is not in progress
                    isTestingCell = False
                else:
                    print(f"unknown command {data[0]}")
            elif data_type == RX_DATA_TYPE_LOGGING_DATA:
                try:
                    if open_circuit_cell_voltage_num_samples == 0:
                        averageOpenCircuitVoltage = 0.0
                    else: 
                        if open_circuit_cell_voltage_num_samples > 10:
                            averageOpenCircuitVoltage = sum(open_circuit_cell_voltages) / 10
                        else:
                            averageOpenCircuitVoltage = sum(open_circuit_cell_voltages) / open_circuit_cell_voltage_num_samples
                    cell_data = get_characterization(data, averageOpenCircuitVoltage)
                    # Update Voltage
                    if abs(cell_data.voltage_V) > ZERO_VOLTAGE_THRESHOLD:
                        window['cell_V'].update(value=f"{cell_data.voltage_V:.3f}")
                        if cell_data.voltage_V < 0:
                            error_message = "DISCONNECT HVD: CELL IS BACKWARDS BAD!!!"
                            print(error_message)
                        elif cell_data.voltage_V >= ZERO_VOLTAGE_THRESHOLD and cell_data.voltage_V < SAMSUMG_30Q_MIN_VOLTAGE:
                            error_message = "DISCONNECT HVD: Cell voltage must be >= 2.5V"
                            print(error_message)
                    
                    # Update Current
                    if cell_data.current_A < -ZERO_CURRENT_THRESHOLD:
                        error_message = "DISCONNECT HVD: CELL IS BACKWARDS BAD!!!"
                        print(error_message)
                    current_average_for_display_sum -= current_average_for_display[current_average_for_display_index]
                    current_average_for_display[current_average_for_display_index] = cell_data.current_A
                    current_average_for_display_sum += current_average_for_display[current_average_for_display_index]
                    current_average_for_display_index = (current_average_for_display_index + 1) % NUM_SAMPLES_IN_CURRENT_AVERAGE
                    window['cell_I'].update(value=f"{(current_average_for_display_sum / NUM_SAMPLES_IN_CURRENT_AVERAGE):.3f}")

                    # Update temperature
                    if abs(cell_data.temperature1_C) > ZERO_TEMPERATURE_THRESHOLD:
                        window['cell_T'].update(value=f"{cell_data.temperature1_C:.2f}")
                    
                    # Update resistance
                    if abs(cell_data.resistance_Ohm) != float("inf"):
                        window['cell_R'].update(value=f"{math.ceil(cell_data.resistance_Ohm*OHMS_TO_MILLIOHMS)}")

                    # Open Circuit Voltage is whenever the current is less than VOLTAGE_OC_CURRENT_THRESHOLD
                    if cell_data.current_A >= 0 and cell_data.current_A <= VOLTAGE_OC_CURRENT_LIMIT:
                        open_circuit_cell_voltages[open_circuit_cell_voltage_num_samples % NUM_SAMPLES_IN_OPEN_CIRCUIT_AVERAGE] = cell_data.voltage_V
                        open_circuit_cell_voltage_num_samples += 1

                except Exception as e:
                    print("Logging Data Error")
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
    if error_message == "None":
        wasFault = False
    else:
        wasFault = True
    window['errors'].update(value=error_message)
    
window.close()
if cell_logfile_csv:
    cell_logfile_csv.close()
