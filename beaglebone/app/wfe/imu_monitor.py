import csv
import board
import adafruit_bno055
import datetime
import time
import os
today = datetime.now().strftime("%b-%d-%y_%H-%M-%S")
logs_folder = "/home/debian/imu_logs/"
imu_file = "{}/{}_imu.csv".format(logs_folder, today)
#library used: https://github.com/adafruit/Adafruit_CircuitPython_BNO055
class IMUMonitor:
    def __init__(self):
        
        i2c = board.I2C() #returns object with board's designated i2c bus(es)
        self.imu = adafruit_bno055.BNO055_I2C(i2c) #creates IMU sensor object for snesnor connected to i2c bus

        #Adds heading row to new csv file 
        if not os.path.isfile(imu_file):
            with open(imu_file, 'w', newline='') as csvfile:
                self.writer = csv.writer(csvfile, delimiter='|')
                self.writer.writerow(['time_stamp', 'linear acceleration', 'magnetomer', 'gyroscope', 'euler angle', 'quaternion', 'acceleration', 'gravity' ])
                #Timestamp format: MM/DD/YYYY hour:min:sec
        print('IMU initilialized. Ready to log data to imu_data.csv')

    def monitor_bus(self):
        while True:

            with open(imu_file, 'a', newline='') as csvfile:
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
                time.sleep(0.05)

def main():
    monitor = IMUMonitor()
    monitor.monitor_bus()

if __name__ == "__main__":
    main()
