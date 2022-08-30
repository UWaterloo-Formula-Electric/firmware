import csv
import serial
import adafruit_bno055
import datetime
import time
import os

#library used: https://github.com/adafruit/Adafruit_CircuitPython_BNO055
class IMUMonitor:
    def __init__(self):
        
        #Opens a port on the beaglebone for uart 
        uart = serial.Serial("/dev/ttyO4", baudrate=9600, timeout=10) #Specifies which port on beaglebone to use for uart: Need to find correct port to use during testing
        self.imu = adafruit_bno055.BNO055_I2C(uart) #Creates IMU sensor object for sensor connected to uart bus
        
        #Adds heading row to new csv file 
        #If no file named imu_data.csv, creates a new file on beaglebone
        if not os.path.isfile('./imu_data.csv'):
            with open('./imu_data.csv', 'w', newline='') as csvfile:
                self.writer = csv.writer(csvfile, delimiter='|')
                self.writer.writerow(['time_stamp', 'linear acceleration', 'magnetomer', 'gyroscope', 'euler angle', 'quaternion', 'acceleration', 'gravity' ])
                #Timestamp format: MM/DD/YYYY hour:min:sec
        print('IMU initilialized. Ready to log data to imu_data.csv')

    def monitor_bus(self):
        while True:

            with open('./imu_data.csv', 'a', newline='') as csvfile:
                writer = csv.writer(csvfile)
                writer.writerow(["{}".format(datetime.datetime.now()), 
                                 "{}".format(self.imu.linear_acceleration),
                                 "{}".format(self.imu.magnetic), 
                                 "{}".format(self.imu.gyro), 
                                 "{}".format(self.imu.euler), 
                                 "{}".format(self.imu.quaternion), 
                                 "{}".format(self.imu.acceleration), 
                                 "{}".format(self.imu.gravity)
                                        ])
                print("Logged IMU data @{}".format(datetime.datetime.now()))
                time.sleep(1)

def main():
    monitor = IMUMonitor()
    monitor.monitor_bus()

if __name__ == "__main__":
    main()
