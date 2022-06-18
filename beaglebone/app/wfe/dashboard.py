import cantools
import csv
import math
import sys
import threading
import time

from PySide2 import QtCore
from PySide2.QtWidgets import QApplication, QWidget
from PySide2.QtCore import Qt
from PySide2.QtGui import QColor, QFont, QFontMetrics, QPainter, QPalette, QPen

from util import default_dbc_path, default_dtc_path
from connect.connect import QueueDataSubscriber

class Dashboard(QWidget):
    
    def __init__(self, app, queue_data, *args, **kwargs):
        super(Dashboard, self).__init__()

        self.queue_data = queue_data

        # If the corresponding properties are greater than these values,
        # their respective dials display in red
        self.OVER_SPEED = 200
        self.OVER_TEMP = 40
        self.OVER_VOLTAGE = 30
        
        # Set window title
        self.setWindowTitle("WFE Dashboard")

        self.width = 800
        self.height = 480

        # Radii of big and small dials
        self.r_big = 320
        self.r_small = 200
        
        # Set window size
        self.setGeometry(10, 10, self.width, self.height)

        # Set window position to center of screen
        qtRectangle = self.frameGeometry()
        centerPoint = app.primaryScreen().availableGeometry().center()
        qtRectangle.moveCenter(centerPoint)
        self.move(qtRectangle.topLeft());

        # Set background to black
        pal = self.palette()
        pal.setColor(QPalette.Background, Qt.black)
        self.setAutoFillBackground(True)
        self.setPalette(pal)

        self.mode_display = TextDisplay(self,
                                        text="Mode: Norm",
                                        colour=Qt.white,
                                        align=Qt.AlignLeft)

        self.battery_display = TextDisplay(self,
                                           text="Battery: 100%",
                                           colour=Qt.green,
                                           align=Qt.AlignRight)

        self.error_display = ErrorDisplay(self, align=Qt.AlignBottom)

        self.speed_display = TextDisplay(self,
                                         text="Speed: 0 kph",
                                         colour=QColor(44, 197, 239),
                                         align=Qt.AlignHCenter)

        self.temp_display = TextDisplay(self,
                                        text="Temp: N/A",
                                        colour=QColor(255, 204, 0),
                                        align=Qt.AlignLeft,
                                        y_offset=45)

        self.voltage_display = TextDisplay(self,
                                           text="Voltage: 0 V",
                                           colour=QColor(181, 124, 255),
                                           align=Qt.AlignRight,
                                           y_offset=45)

        self.speed_dial = Dial(norm_colour=QColor(44, 197, 239),
                               over_colour=Qt.red,
                               x=self.width//2 - self.r_big//2,
                               y=100,
                               radius=self.r_big,
                               thickness=20,
                               min_val=0,
                               max_val=280,
                               over_val=self.OVER_SPEED,
                               start_ang=210,
                               span_ang=-240,
                               text="kph",
                               text_x_offset=0)

        self.temp_dial = Dial(norm_colour=QColor(255, 204, 0),
                              over_colour=Qt.red,
                              x=90,
                              y=200,
                              radius=self.r_small,
                              thickness=15,
                              min_val=-20,
                              max_val=80,
                              over_val=self.OVER_TEMP,
                              start_ang=210,
                              span_ang=-140,
                              text="°C",
                              text_x_offset=-20)
        
        self.voltage_dial = Dial(norm_colour=QColor(181, 124, 255),
                                 over_colour=Qt.red,
                                 x=self.width-90-self.r_small,
                                 y=200,
                                 radius=self.r_small,
                                 thickness=15,
                                 min_val=-20,
                                 max_val=60,
                                 over_val=self.OVER_VOLTAGE,
                                 start_ang=-30,
                                 span_ang=140,
                                 text="V",
                                 text_x_offset=20)

        self.current_dtc_messages = []

        # Update values every 10 milliseconds
        self.timer = QtCore.QTimer()
        self.timer.setInterval(10)
        self.timer.timeout.connect(self.update)
        self.timer.start()
        self.show()

    # Update error text display by checking messages in DTC payload
    # Sets colour of text depending on severity
    def __update_error_display(self, dtc_message_payload):
        for message in dtc_message_payload:
            severity, text = message["severity"], message["message"]
            self.error_display.add_error_message(text, int(severity))

    def update(self):
        # TODO: update this to read data from zmq message queue
        CAN_data = self.queue_data.fetch()

        self.__update_error_display(CAN_data["dtc_message_payload"])
        self.queue_data.push("dtc_message_payload", [])

        battery_val = CAN_data["battery"]
        self.battery_display.set_text("Battery: {}%".format(battery_val))
        if battery_val > 66:
            self.battery_display.set_colour(Qt.green)
        elif battery_val > 33:
            self.battery_display.set_colour(Qt.yellow)
        else:
            self.battery_display.set_colour(Qt.red)

        self.speed_display.set_text("Speed: {} kph".format(CAN_data["speed"]))
        self.temp_display.set_text("Temp: {}°C".format(CAN_data["temperature"]))
        self.voltage_display.set_text("Voltage: {} V".format(CAN_data["voltage"]))

        self.speed_dial.set_value(CAN_data["speed"])
        self.temp_dial.set_value(CAN_data["temperature"])
        self.voltage_dial.set_value(CAN_data["voltage"])

        self.repaint()


    # Repaints canvas, called automatically with self.repaint()
    def paintEvent(self, event):
        qp = QPainter(self)

        self.mode_display.draw(qp)
        self.battery_display.draw(qp)
        self.error_display.draw(qp)
        self.speed_display.draw(qp)
        self.temp_display.draw(qp)
        self.voltage_display.draw(qp)
        self.speed_dial.draw(qp)
        self.temp_dial.draw(qp)
        self.voltage_dial.draw(qp)


class Dial:
    """ Used to display speed, temp, and voltage. """
    
    def __init__(self, norm_colour, over_colour,
                 x, y, radius, thickness, 
                 min_val, max_val, over_val, 
                 start_ang, span_ang,
                 text, text_x_offset):

        self.norm_colour = norm_colour
        self.over_colour = over_colour

        self.x = x
        self.y = y
        # Radius of arc
        self.radius = radius
        # Thickness of arc
        self.thickness = thickness
        
        self.min_val = min_val
        self.max_val = max_val
        self.over_val = over_val
        self.val = min_val

        # In PyQt/PySide, angles in degrees must be multiplied by 16
        self.start_ang = start_ang * 16
        self.span_ang = span_ang * 16

        self.text = text
        self.text_x_offset = text_x_offset

        self.is_over = (self.val >= self.over_val)

        self.val_font_size = self.radius // 8
        self.text_font_size = self.radius // 16


    # Drawing dial
    def draw(self, qp):

        # Grey arc, shows total span of dial
        qp.setPen(QPen(Qt.darkGray, self.thickness, Qt.SolidLine))
        qp.drawArc(self.x, self.y, self.radius, self.radius, self.start_ang, self.span_ang)        

        pen = QPen(self.over_colour if self.is_over else self.norm_colour,
                   self.thickness, Qt.SolidLine)
        qp.setPen(pen)
        span_ang = ((self.val - self.min_val) / (self.max_val - self.min_val)) * self.span_ang

        # Coloured arc on top of grey arc, shows current span of dial
        qp.drawArc(self.x, self.y, self.radius, self.radius, self.start_ang, span_ang)

        qp.setPen(Qt.white)
        qp.setFont(QFont('Arial', self.val_font_size))

        # Display value
        qp.drawText(self.x + self.text_x_offset,
                    self.y - self.radius * 0.075,
                    self.radius,
                    self.radius,
                    Qt.AlignCenter,
                    str(self.val))

        qp.setPen(self.over_colour if self.is_over else self.norm_colour)
        qp.setFont(QFont('Arial', self.text_font_size))

        # Display smaller text (the unit) e.g. "kph" in speed dial
        qp.drawText(self.x + self.text_x_offset,
                    self.y + self.radius * 0.075,
                    self.radius,
                    self.radius,
                    Qt.AlignCenter,
                    self.text)

    def set_value(self, v):
        self.val = v
        self.is_over = (self.val >= self.over_val)


class TextDisplay:
    """ Used to display text. """

    def __init__(self, parent, text, colour, align, font_size=22, y_offset=0):
        self.parent = parent
        self.text = text
        self.colour = colour
        self.align = align
        self.font = QFont('Arial', font_size)
        
        # Creates padding around window edge
        self.x = self.parent.width * 0.05 / 2
        self.y = self.parent.height * 0.05 / 2 + y_offset
        self.width = self.parent.width * 0.95
        self.height = self.parent.height * 0.95

    def set_text(self, text):
        self.text = text

    def set_colour(self, colour):
        self.colour = colour

    def draw(self, qp):
        qp.setPen(self.colour)
        qp.setFont(self.font)
        metrics = QFontMetrics(self.font)
        elided_text  = metrics.elidedText(self.text, QtCore.Qt.ElideRight, self.width)
        qp.drawText(self.x, self.y, self.width, self.height, self.align, elided_text)

class ErrorDisplay:
    """ Used to display DTC messages. """

    def __init__(self, parent, align):
        self.parent = parent
        self.align = align
        self.font = QFont('Arial', 20)
        self.error_messages = []
        self.severities = []

        # Creates padding around window edge
        self.x = self.parent.width * 0.05 / 2
        self.y = self.parent.height * 0.05 / 2
        self.width = self.parent.width * 0.95
        self.height = self.parent.height * 0.95

        self.severity_settings = {
            1: { "header": "F", "colour": Qt.red },
            2: { "header": "C", "colour": QColor(255, 103, 0) },
            3: { "header": "E", "colour": QColor(255, 193, 7) },
            4: { "header": "W", "colour": Qt.yellow }
        }

    def add_error_message(self, err_msg, severity):
        if len(self.error_messages) == 3:
            self.error_messages.pop(0)
            self.severities.pop(0)
        try:
            header = self.severity_settings[severity]["header"]
        except KeyError:
            header = "U"
        self.error_messages.append("{}: {}".format(header, err_msg))
        self.severities.append(severity)

    def draw(self, qp):
        qp.setFont(self.font)

        # If there are no error messages
        if not self.error_messages:
            qp.setPen(Qt.white)
            qp.drawText(self.x, self.y, self.width, self.height, self.align, "No DTC Messages Received")

        num_error_msgs = len(self.error_messages)
        for i in range(num_error_msgs):
            error_msg = self.error_messages[i]
            severity = self.severities[i]
            colour = self.severity_settings[severity]["colour"]
            qp.setPen(colour)
            metrics = QFontMetrics(self.font)
            elided_text  = metrics.elidedText(error_msg, QtCore.Qt.ElideRight, self.width)
            y = self.y - (num_error_msgs - 1 - i) * metrics.height()
            qp.drawText(self.x, y, self.width, self.height, self.align, elided_text)


class QueueData:
    """ Facilitates data transfer between the GUI and the Queue. """

    def __init__(self):
        self._lock = threading.Lock()
        self._data = {
            "dtc_message_payload": [],
            "speed": 0,
            "temperature": 0,
            "voltage": 0,
            "battery": 0
        }

    def fetch(self):
        with self._lock:
            return self._data

    def push(self, key, value):
        if key not in self._data:
            raise RuntimeError("Queue data schema violated")

        with self._lock:
            self._data[key] = value


class QueueThread(threading.Thread):
    """ Collects data from zmq message queue in the background. """

    em_enable_fail_codes = [16, 17]
    em_enable_fail_reasons = ["bpsState false", "low brake pressure", "throttle non-zero",
                              "brake not pressed", "not hv enabled", "motors failed to start"]

    def __init__(self, queue_data, dbc=default_dbc_path(), dtc=default_dtc_path()):
        threading.Thread.__init__(self)
        self.queue_data = queue_data
        self.dashboard_subscriber = DashboardSubscriber()
        self.db = cantools.database.load_file(dbc)

        self.dtc_messages = []
        with open(dtc) as dtc_file:
            csv_reader = csv.reader(dtc_file)
            next(csv_reader)                        # Read header
            for row in csv_reader:
                self.dtc_messages.append(row[6])    # Add each DTC message to list

    # Convert motor RPM to km/h
    def get_speed_kph_from_rpm(self, new_speed_rpm, current_speed_kph):
        radius = 9              # Inches (wheel radius)
        in_to_km = 0.001524     # Inches to km conversion
        sprocket_ratio = 52/15  # Motor sprocket to wheel sprocket ratio
        new_speed_kph = (new_speed_rpm * in_to_km * sprocket_ratio * 2 * math.pi * radius)
        return (0.5 * (current_speed_kph + new_speed_kph))

    def run(self):
        speed = 0
        while True:
            can_packet = self.dashboard_subscriber.recv()

            # BMU_stateBatteryHV
            if can_packet["frame_id"] == self.db.get_message_by_name("BMU_stateBusHV").frame_id:
                voltage = can_packet["signals"]["VoltageCellMin"]
                #self.queue_data.push("voltage", round(voltage, 1))
                self.queue_data.push("voltage", round(voltage,2))

            # BMU_batteryStatusHV
            elif can_packet["frame_id"] == self.db.get_message_by_name("BMU_batteryStatusHV").frame_id:
                battery = can_packet["signals"]["StateBatteryChargeHV"]
                self.queue_data.push("battery", round(battery, 1))
                temp = can_packet["signals"]["TempCellMax"]
                self.queue_data.push("temperature", round(temp, 2))
 
            # SpeedFeedbackRight, SpeedFeedbackLeft
            elif can_packet["frame_id"] in [self.db.get_message_by_name("SpeedFeedbackRight").frame_id,
                                            self.db.get_message_by_name("SpeedFeedbackLeft").frame_id]:
                current_speed = 0
                if can_packet["frame_id"] == self.db.get_message_by_name("SpeedFeedbackRight").frame_id:
                    current_speed = can_packet["signals"]["SpeedMotorRight"]
                else:
                    current_speed = can_packet["signals"]["SpeedMotorLeft"]
                speed = self.get_speed_kph_from_rpm(current_speed, speed)
                self.queue_data.push("speed", round(speed, 1))

            # PDU_DTC, DCU_DTC, VCU_F7_DTC, BMU_DTC
            elif can_packet["frame_id"] in [self.db.get_message_by_name("PDU_DTC").frame_id,
                                            self.db.get_message_by_name("DCU_DTC").frame_id,
                                            self.db.get_message_by_name("VCU_F7_DTC").frame_id,
                                            self.db.get_message_by_name("BMU_DTC").frame_id]:
                code, severity, data = can_packet["signals"]["DTC_CODE"], can_packet["signals"]["DTC_Severity"], can_packet["signals"]["DTC_Data"]
                try:
                    message = self.dtc_messages[int(code)-1]
                except IndexError:
                    message = "Unknown DTC Code: {})".format(code)

                # Substitute #data in message with actual error reason
                if code in QueueThread.em_enable_fail_codes:
                    message = message[:message.find(" (Reasons")]
                    message = message.replace("#data", QueueThread.em_enable_fail_reasons[data])
                elif "#data" in message:
                    message = message.replace("#data", data)

                payload = [{
                    "severity": severity,
                    "message": message
                }]
                self.queue_data.push("dtc_message_payload", payload)

class DashboardSubscriber(QueueDataSubscriber):
    """ Subscribes dashboard to data using zmq """

    def __init__(self):
        super(DashboardSubscriber, self).__init__()
        self.subscribe_to_packet_type("")


def main():
    # Start thread in background to collect data
    data = QueueData()
    queue_thread = QueueThread(data)
    queue_thread.daemon = True
    queue_thread.start()

    app = QApplication(sys.argv)

    Dashboard(app, data)

    # Run application until user closes it
    sys.exit(app.exec_())

if __name__ == "__main__":
     main()
