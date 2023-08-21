import math
import csv
from io import StringIO

from serial.tools import list_ports
from serial import Serial
import pandas as pd

import PySimpleGUI as sg

from CellData import CellData
from constants import VOLTAGE_OC_CURRENT_LIMIT

# Find port and connect
ct_port = None
for port, desc, hwid in list_ports.comports():
    if "STLink" in desc:
        ct_port = Serial(port, 230400, timeout=None)
        break

assert ct_port is not None, "No STLink found"


def set_current_target(current: float):
    ct_port.write(f"{current:.3}\n".encode("ascii"))


def get_characterization():
    # Expected format: "timestamp, voltage, current, temperature"
    raw_data = ct_port.read(ct_port.in_waiting)
    # # find the text between last 2 newlines
    last_newline = raw_data.rfind(b"\n")
    second_last_newline = raw_data.rfind(b"\n", 0, last_newline)
    raw_data = raw_data[second_last_newline+1:last_newline].decode("ascii")
    data = raw_data.split(", ")
    try:
        return int(data[0]), float(data[1]), float(data[2]), float(data[3])
    except ValueError:
        if raw_data.startswith("desired current set to"):
            return get_characterization()
        raise
    except IndexError:
        raise ValueError(f"Invalid data: {raw_data}")


cell_df = pd.DataFrame(columns=["Time Stamp", "Cell Open Circuit Voltage",
                                "Cell Voltage", "Cell Current", "Cell Temperature", "Cell Internal Resistance"])

csv_file = open("../CellData/continuous_cell_data.csv", "w", newline='')
writer = csv.writer(csv_file)
writer.writerow(cell_df.columns)
isCharacterization = False


sg.theme('DarkGrey14')
# All the stuff inside your window.
layout = [
    [sg.Frame('Meters', [
        [
            sg.Frame('Voltage [V]', [[sg.Text(
                '0.0', key='cell_V', s=10, justification='center', font=('Consolas', 30))]]),
            sg.Frame('Current [A]', [[sg.Text('0.0', key='cell_I',
                                              s=10, justification='center', font=('Consolas', 30))]])
        ],
        [
            sg.Frame('Temperature [°C]', [[sg.Text(
                '0.0', key='cell_T', s=10, justification='center', font=('Consolas', 30))]]),
            sg.Frame('Derived Internal Resitance [Ω]', [[sg.Text(
                '0.0', key='cell_R', s=10, justification='center', font=('Consolas', 30))]])
        ]])
    ],
    [
        sg.Frame('Request Current Draw', [
            [sg.InputText('0.0', key='requested_curr_draw',
                          size=18, justification='center')],
            [sg.Button('Request', size=15)]
        ]),
        sg.Frame('Characterization', [
            [sg.StatusBar('False', key='characterization',
                     justification='center', s=13, font=('Consolas'))],
            [sg.Button('Start', size=6), sg.Button('Stop', size=6)]
        ]),
        sg.Frame("Save Characterization", [
            [sg.Text('File Name:'), sg.InputText('cell_1.csv', key='file_name', size=15)],
            [sg.Button('Save Characterization', size=19)]
        ])
    ],   
]

# Create the Window
window = sg.Window('Cell Tester', layout, size=(500, 300))
# Event Loop to process "events" and get the "values" of the inputs
while True:
    event, values = window.read(timeout=10)  # type: ignore
    if event == sg.WIN_CLOSED:
        break
    elif event == 'Start':
        isCharacterization = True
        window['characterization'].update(value='True')
    elif event == 'Stop':
        isCharacterization = False
        window['characterization'].update(value='False')
    elif event == 'Save Characterization':
        file_name = values['file_name']
        cell_df.to_csv(f'../CellData/{file_name}',
                       index=False, lineterminator='\n')
        cell_df.drop(cell_df.index, inplace=True)  # reset dataframe
        cell_number = int(file_name.split("_")[1].split(".")[0])
        window['file_name'].update(value=f"cell_{cell_number+1}.csv")
    elif event == 'Request Current Draw':
        set_current_target(float(values['requested_curr_draw']))
    else:
        # Get data
        try:
            cell_data = CellData(*get_characterization())
        except ValueError:
            continue
        print(cell_data)
        # Update values
        window['cell_V'].update(value=f"{cell_data.voltage:.3f}")
        window['cell_I'].update(value=f"{cell_data.current:.3f}")
        window['cell_T'].update(value=f"{cell_data.temperature:.2f}")

        # Open Circuit Voltage is whenever the current is less than VOLTAGE_OC_CURRENT_THRESHOLD
        cell_data.voltage_open_circuit = cell_df.loc[cell_df["Cell Current"] <=
                                                     VOLTAGE_OC_CURRENT_LIMIT]["Cell Voltage"].mean()
        window['cell_R'].update(value=cell_data.resistance)

        # Log data
        if isCharacterization:
            cell_df.loc[len(cell_df)] = cell_data.formatted_data()
        writer.writerow(cell_data.formatted_data())

window.close()
csv_file.close()
