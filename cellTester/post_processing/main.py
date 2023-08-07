import csv
import time
import PySimpleGUI as sg
from serial.tools import list_ports
from serial import Serial
import pandas as pd

# Find port and connect
ct_port = None
for port, desc, hwid in list_ports.comports():
    if "STLink" in desc:
        ct_port = Serial(port, 230400, timeout=1)
        break

# assert ct_port is not None, "No STLink found"

def start_new_characterization():
    ct_port.write(b"Start\n")


def stop_characterization():
    ct_port.write(b"Stop\n")

def get_characterization():
    # Expected format: "time, isCharacterization, voltage, current, temperature"
    # Uncomment for real data
    # data = ct_port.readline().decode("ascii").strip()
    data = "13502, 1, 4.000, 2.000, 25.00"

    data = data.split(", ")
    return int(data[0]), bool(int(data[1])), float(data[2]), float(data[3]), float(data[4])
    
csv_df = pd.DataFrame(columns=["Time Stamp", "isCharacterization", "Cell Voltage", "Cell Current", "Cell Temperature"])

csv_file = open("../CellData/continuous_cell_data.csv", "w", newline='')
writer = csv.writer(csv_file)
writer.writerow(["Time Stamp", "isCharacterization", "Cell Voltage", "Cell Current", "Cell Temperature"])

sg.theme('Dark')
# All the stuff inside your window.
layout = [
    [sg.Text('Characterization:'), sg.Text('False', key='characterization')],
    [sg.Text('Voltage [V]:'), sg.Text('0.0', key='cell_V')],
    [sg.Text('Current [I]:'), sg.Text('0.0', key='cell_I')],
    [sg.Text('Calculated Resitance [Ω]:'), sg.Text('0.0', key='cell_R')],
    [sg.Text('Temperature [°C]:'), sg.Text('0.0', key='cell_T')],
    [sg.Button('Start'), sg.Button('Stop')],
    [sg.Text('File Name:'), sg.InputText('cell_1.csv', key='file_name')],
    [sg.Button('Save Characterization')]
    
]

# Create the Window
window = sg.Window('Cell Tester', layout, size=(800, 600))
# Event Loop to process "events" and get the "values" of the inputs
while True:
    event, values = window.read(timeout=0)  # type: ignore
    if event == sg.WIN_CLOSED:
        break
    elif event == 'Start':
        start_new_characterization()
    elif event == 'Stop':
        stop_characterization()
    elif event == 'Save Characterization':
        file_name = values['file_name']
        csv_df.to_csv(f'../CellData/{file_name}', index=False, lineterminator='\n')
        csv_df.drop(csv_df.index, inplace=True) # reset dataframe
        cell_number = int(file_name.split("_")[1].split(".")[0])
        window['file_name'].update(value=f"cell_{cell_number+1}.csv")
    else:
        # Get data
        data = get_characterization()        
        # Update values
        time_stamp, isCharacterization, v, i, temp = data
        window['cell_V'].update(value=v)
        window['cell_I'].update(value=i)
        window['cell_T'].update(value=temp)
        window['cell_R'].update(value=v/(i+1e-10)) # Probably update this with some better offset
        window['characterization'].update(value=isCharacterization)
        # Log data
        if isCharacterization:
            csv_df.loc[len(csv_df)] = data # type: ignore
        writer.writerow(data)

window.close()
csv_file.close()
