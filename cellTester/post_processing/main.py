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
    return time.time(), 0, 0, 0
    # return ct_port.readline().decode("utf-8").strip()


sg.theme('Dark')
# All the stuff inside your window.
layout = [
    [sg.Text('Voltage [V]:'), sg.Text('0.0', key='cell_V')],
    [sg.Text('Current [I]:'), sg.Text('0.0', key='cell_I')],
    [sg.Text('Calculated Resitance [Ω]:'), sg.Text('0.0', key='cell_R')],
    [sg.Text('Temperature [°C]:'), sg.Text('0.0', key='cell_T')],
    [sg.Button('Start'), sg.Button('Stop')]
]

# Create the Window
window = sg.Window('Cell Tester', layout, size=(800, 600))
# Event Loop to process "events" and get the "values" of the inputs

while True:
    event, values = window.read(timeout=10)  # type: ignore
    if event == sg.WIN_CLOSED:
        break
    elif event == 'Start':
        start_new_characterization()
    elif event == 'Stop':
        stop_characterization()
    else:
        # Update values
        window['cell_V'].update(value=v)
        window['cell_I'].update(value=i)
        window['cell_T'].update(value=temp)
        window['cell_R'].update(value=v/(i+1e-6))
        # print(get_characterization())

window.close()
