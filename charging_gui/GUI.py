from PyQt6.QtWidgets import QApplication, QWidget, QLabel, QVBoxLayout, QPushButton
import pyqtgraph as pg
import numpy as np


# simulate real time data for now
# replace with real time CAN data later
def update():
    global voltages, temperatures, voltage_bar, temp_bar
    voltages = np.random.uniform(0, 6.55, 140)
    temperatures = np.random.uniform(15, 60, 140)
    voltage_plot.removeItem(voltage_bar)
    temperature_plot.removeItem(temp_bar)

    voltage_bar = pg.BarGraphItem(x=x, height = voltages, width=0.8, brush='green')
    temp_bar = pg.BarGraphItem(x=x, height=temperatures, width=0.8, brush='green')
    voltage_plot.addItem(voltage_bar)
    temperature_plot.addItem(temp_bar)

app = QApplication([])

window = QWidget()
window.setWindowTitle("Charging GUI")
window.setGeometry(100, 100, 1000, 700)
layout = QVBoxLayout()
window.setLayout(layout)

# set up bar chart for temperature and voltage
voltage_plot = pg.PlotWidget(title='cell voltage')
voltage_plot.setLabel("left", 'voltage', units='V')
voltage_plot.setLabel("bottom", 'Cell Index')
voltage_plot.setYRange(0, 6.55)
layout.addWidget(voltage_plot)

temperature_plot = pg.PlotWidget(title='cell temperature')
temperature_plot.setLabel("left", "voltage", units="°C")
temperature_plot.setLabel("bottom", "Cell Index")
temperature_plot.setYRange(15, 60)
layout.addWidget(temperature_plot)

x = np.arange(140)  
voltages = np.random.uniform(3.0, 4.2, 140)
temperatures = np.random.uniform(20, 40, 140)

voltage_bar = pg.BarGraphItem(x=x, height=voltages, width=0.8, brush='green')
temp_bar = pg.BarGraphItem(x=x, height=temperatures, width=0.8, brush='green')

voltage_plot.addItem(voltage_bar)
temperature_plot.addItem(temp_bar)

x = np.arange(140)
voltages = np.random.uniform(0, 6.55, 140)
temperatures = np.random.uniform(15, 60, 140)

# update the GUI every second
timer = pg.QtCore.QTimer()
timer.timeout.connect(update)
timer.start(1000)
window.show()
app.exec()

