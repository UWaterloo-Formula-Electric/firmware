from PyQt6.QtWidgets import QApplication, QWidget, QLabel, QGridLayout
from PyQt6.QtCore import Qt
from PyQt6.QtGui import QFont
import pyqtgraph as pg
from readCAN import CANBMUMonitor



class GUI():
    def __init__(self, BMU_MONITOR):
        self.BMU_MONITOR = BMU_MONITOR
        self.voltage_threshold = 3.0
        self.temp_threshold_h = 45
        self.temp_threshold_l = 30
        self.update_time = 1000

        # set up the widget for the GUI
        self.labels = []
        self.app = QApplication([])
        self.window = QWidget()
        self.window.setWindowTitle("Charging GUI")
        self.window.setGeometry(100, 100, 1000, 700)
        self.layout = QGridLayout()
        self.window.setLayout(self.layout)

        self.add_boxes()
        self.timer = pg.QtCore.QTimer()
        # refresh every second
        self.timer.timeout.connect(self.update)
        self.timer.start(self.update_time)




    def add_boxes(self):

        # use generated data to test for now 
        self.BMU_MONITOR.get_message()
        self.BMU_MONITOR.process_can_message()
        voltages, temperatures = self.BMU_MONITOR.voltage_channels, self.BMU_MONITOR.temperature_channels

        for i in range(140):
            row = i%20
            col = i // 20
            voltage = voltages[i]
            voltage_color = None
            temperature = temperatures[i]
            temperature_color = None
            # setting voltage/temperature color
            if voltage > self.voltage_threshold:
                voltage_color = "green"
            else:
                voltage_color = "red"

            if temperature > self.temp_threshold_h:
                temperature_color = "red"
            elif temperature > self.temp_threshold_l:
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
            self.labels.append(label)
            self.layout.addWidget(label, row, col)
        

    def update(self):
        self.BMU_MONITOR.get_message()
        self.BMU_MONITOR.process_can_message()
        voltages, temperatures = self.BMU_MONITOR.voltage_channels, self.BMU_MONITOR.temperature_channels
        for index, label in enumerate(self.labels):
            voltage = voltages[index]
            temperature = temperatures[index]
            if voltage > self.voltage_threshold:
                voltage_color = "green"
            else:
                voltage_color = "red"

            if temperature > self.temp_threshold_h:
                temperature_color = "red"
            elif temperature > self.temp_threshold_l:
                temperature_color = 'yellow'
            else:
                temperature_color = 'green'

            label.setText(
                    f"<span style='color: {voltage_color};'>{voltages[index]} V</span><br>"
                    f"<span style='color: {temperature_color};'>{temperatures[index]}°C</span>"
            )    

    def start(self):
        self.window.show()
        self.app.exec()


def main():
    monitor = CANBMUMonitor()
    gui = GUI(monitor)
    gui.start()

main()

