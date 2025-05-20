from PyQt6.QtWidgets import QApplication, QWidget, QLabel, QVBoxLayout, QPushButton, QGroupBox, QGridLayout
import numpy as np
from PyQt6.QtCore import Qt
from PyQt6.QtGui import QFont
import pyqtgraph as pg

# simulate real time data for now
# replace with real time CAN data later
labels = []
voltage_threshold = 3.0
temp_threshold_h = 45
temp_threshold_l = 30

def update():
    voltages = np.round(np.random.uniform(3.0, 4.2, 140), 1)
    temperatures = np.round(np.random.uniform(0, 60, 140), 1)
    for index, label in enumerate(labels):
        voltage = voltages[index]
        temperature = temperatures[index]
        if voltage > voltage_threshold:
            voltage_color = "green"
        else:
            voltage_color = "red"

        if temperature > temp_threshold_h:
            temperature_color = "red"
        elif temperature > temp_threshold_l:
            temperature_color = 'yellow'
        else:
            temperature_color = 'green'

        label.setText(
                f"<span style='color: {voltage_color};'>{voltages[index]} V</span><br>"
                f"<span style='color: {temperature_color};'>{temperatures[index]}°C</span>"
        )        

def main():
    app = QApplication([])

    window = QWidget()
    window.setWindowTitle("Charging GUI")
    window.setGeometry(100, 100, 1000, 700)
    layout = QGridLayout()
    window.setLayout(layout)

    x = np.arange(140)  
    voltages = np.round(np.random.uniform(0, 6.55, 140), 1)
    temperatures = np.round(np.random.uniform(20, 40, 140), 1)

    
    for i in range(140):
        row = i%20
        col = i // 20
        voltage = voltages[i]
        voltage_color = None
        temperature = temperatures[i]
        temperature_color = None
        # setting voltage color
        if voltage > voltage_threshold:
            voltage_color = "green"
        else:
            voltage_color = "red"

        if temperature > temp_threshold_h:
            temperature_color = "red"
        elif temperature > temp_threshold_l:
            temperature_color = 'yellow'
        else:
            temperature_color = 'green'

        label = QLabel()
        label.setText(
            f"<span style='color: {voltage_color};'>{voltages[i]} V</span><br>"
            f"<span style='color: {temperature_color};'>{temperatures[i]}°C</span>"
        )
        label.setFixedSize(150, 30)
        label.setStyleSheet("border: 1px solid black; background-color: black;")
        label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        label.setFont(QFont("Arial", 8))
        labels.append(label)
        layout.addWidget(label, row, col)
    

   

    timer = pg.QtCore.QTimer()
    timer.timeout.connect(update)
    timer.start(1000)
# update the GUI every second
    window.show()
    app.exec()

main()

