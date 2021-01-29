#!/usr/bin/python3

from PySide2.QtWidgets import *
from PySide2.QtGui import *
from PySide2.QtCore import *

import sys
import math
import random
import threading

from time import sleep

class Dashboard(QWidget):
    
    def __init__(self, app, queue_data, *args, **kwargs):
        super(Dashboard, self).__init__()

        self.queue_data = queue_data

        # if the corresponding properties are greater than these values,
        # their respective dials display in red
        self.OVER_SPEED = 200
        self.OVER_TEMP = 40
        self.OVER_VOLTAGE = 30
        
        # set window title
        self.setWindowTitle("WFE Dashboard")

        self.width = 800
        self.height = 480

        # radii of big and small dials
        self.r_big = 320
        self.r_small = 200
        
        # set window size
        self.setGeometry(10, 10, self.width, self.height)

        # set window position to center of screen
        qtRectangle = self.frameGeometry()
        centerPoint = app.primaryScreen().availableGeometry().center()
        qtRectangle.moveCenter(centerPoint)
        self.move(qtRectangle.topLeft());

        # set background to black
        pal = self.palette()
        pal.setColor(QPalette.Background, Qt.black)
        self.setAutoFillBackground(True)
        self.setPalette(pal)

        self.mode_display = TextDisplay(self,
                                        label="Mode",
                                        value="Normal",
                                        colour=Qt.white,
                                        align=Qt.AlignLeft)

        self.battery_display = TextDisplay(self,
                                           label="Battery",
                                           value="100%",
                                           colour=Qt.green,
                                           align=Qt.AlignRight)

        self.error_display = TextDisplay(self,
                                         label="Errors",
                                         value="None",
                                         colour=Qt.white,
                                         align=Qt.AlignBottom)

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
        

        # update values every 100 milliseconds
        self.timer = QTimer()
        self.timer.setInterval(100)
        self.timer.timeout.connect(self.update)
        self.timer.start()

        self.show()

    def update(self):
        # TODO: update this to read data from zmq message queue
        CAN_data = self.queue_data.fetch()
        for message in CAN_data["dtc_message_payload"]:
            pass
            # TODO: display error messages, change error message colour
            # if there_are_errors:
            #     self.error_display.set_value(error_message)
            #     self.error_display.set_colour(Qt.red)

        battery_val = round(float(CAN_data["battery"])*100, 1)
        self.battery_display.set_value("{}%".format(battery_val))
        if battery_val > 66:
            self.battery_display.set_colour(Qt.green)
        elif battery_val > 33:
            self.battery_display.set_colour(Qt.yellow)
        else:
            self.battery_display.set_colour(Qt.red)

        self.speed_dial.set_value(round(float(CAN_data["speed"]), 1))
        self.temp_dial.set_value(round(float(CAN_data["temperature"]), 1))
        self.voltage_dial.set_value(round(float(CAN_data["voltage"]), 1))

        self.repaint()


    # repaints canvas, called automatically with self.repaint()
    def paintEvent(self, event):
        qp = QPainter(self)

        self.mode_display.draw(qp)
        self.battery_display.draw(qp)
        self.error_display.draw(qp)
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
        # radius of arc
        self.radius = radius
        # thickness of arc
        self.thickness = thickness
        
        self.min_val = min_val
        self.max_val = max_val
        self.over_val = over_val
        self.val = min_val

        # in PyQt, angles in degrees must be multiplied by 16
        self.start_ang = start_ang * 16
        self.span_ang = span_ang * 16

        self.text = text
        self.text_x_offset = text_x_offset

        self.is_over = (self.val >= self.over_val)

        self.val_font_size = self.radius // 8
        self.text_font_size = self.radius // 16


    # drawing dial
    def draw(self, qp):

        # grey arc, shows total span of dial
        qp.setPen(QPen(Qt.darkGray, self.thickness, Qt.SolidLine))
        qp.drawArc(self.x, self.y, self.radius, self.radius, self.start_ang, self.span_ang)        

        pen = QPen(self.over_colour if self.is_over else self.norm_colour,
                   self.thickness, Qt.SolidLine)
        qp.setPen(pen)
        span_ang = ((self.val - self.min_val) / (self.max_val - self.min_val)) * self.span_ang

        # coloured arc on top of grey arc, shows current span of dial
        qp.drawArc(self.x, self.y, self.radius, self.radius, self.start_ang, span_ang)

        qp.setPen(Qt.white)
        qp.setFont(QFont('Arial', self.val_font_size))

        # display value
        qp.drawText(self.x + self.text_x_offset,
                    self.y - self.radius * 0.075,
                    self.radius,
                    self.radius,
                    Qt.AlignCenter,
                    str(self.val))

        qp.setPen(self.over_colour if self.is_over else self.norm_colour)
        qp.setFont(QFont('Arial', self.text_font_size))

        # display smaller text (the unit) e.g. "kph" in speed dial
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

    def __init__(self, parent, label, value, colour, align):
        self.parent = parent
        self.label = label
        self.value = value
        self.colour = colour
        self.align = align
        
        # creates padding around window edge
        self.x = self.parent.width * 0.05 / 2
        self.y = self.parent.height * 0.05 / 2
        self.width = self.parent.width * 0.95
        self.height = self.parent.height * 0.95

    def set_value(self, value):
        self.value = value

    def set_colour(self, colour):
        self.colour = colour

    def draw(self, qp):
        qp.setPen(self.colour)
        qp.setFont(QFont('Arial', 22))
        text = "{}: {}".format(self.label, self.value)
        qp.drawText(self.x, self.y, self.width, self.height, self.align, text)


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
    """ Collects data from zmq message queue in the "background". """

    def __init__(self, queue_data):
        threading.Thread.__init__(self)
        self.queue_data = queue_data
        # TODO: initialize zmq, subscribe to queue

    def run(self):
        while 1:
            # TODO: fetch queue data
            # TODO: parse queue data, send as appropriate
            self.queue_data.push("speed", 20)
            self.queue_data.push("temperature", 30.4)
            self.queue_data.push("voltage", 12)
            self.queue_data.push("battery", 0.623)
            payload = [
                "CAN Message: Test\n",
                "CAN Message: Test\n"
            ]

            self.queue_data.push("dtc_message_payload", payload)
            sleep(2)
            
class DashboardRunner:

    def run(self):
        # Start thread in background to collect data
        data = QueueData()
        queue_thread = QueueThread(data)
        queue_thread.daemon = True
        queue_thread.start()

        app = QApplication(sys.argv)

        Dashboard(app, data)

        # run application until user closes it
        sys.exit(app.exec_())


