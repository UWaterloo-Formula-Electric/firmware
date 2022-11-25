import csv
from curses import baudrate
from socket import timeout
import serial
import adafruit_gps
import time
import os

class GPSMonitor: 
#init
    def __init__(self):

        #opens a port on the beaglebone for uart 
        uart = serial.Serial("/dev/ttyO4", baudrate=9600, timeout=10)
        # Creating a GPS module instance.
        self.gps = adafruit_gps.GPS(uart) 

        #indicating which sentence (kind of information) we want to read from the GPS
        #0 = off, 1 = on
        #GGA sentences include: UTC (time_stamp), latitude, longitude, GPS quality indicator (fix_quality), 
        # number of satellites in view (no_satellites), antenna altitude above/below mean-sea-level (altitude) 
        # GGA includes other data not needed for our purposes
        #for more information: https://opencpn.org/wiki/dokuwiki/doku.php?id=opencpn:opencpn_user_manual:advanced_features:nmea_sentences

        gll = 0
        rmc = 0
        vtg = 0 
        gga = 1
        gsa = 0 
        gsv = 0

        #tells gps which sentence(s) we want to read 
        #trailing zeros are needed to match expected format of gps commands
        sentences = bytes("PMTK314,{},{},{},{},{},{},0,0,0,0,0,0,0,0,0,0,0,0,0,0,0".format(gll, rmc, vtg, gga, gsa, gsv),encoding='utf8')

        self.gps.send_command(sentences)

        # Set update rate to once a second (1hz). The range is 1 Hz to 10Hz. 
        self.gps.send_command(b"PMTK220,1000")

        #Adds heading row to new csv file 
        if not os.path.isfile('./gps_data.csv'):
            with open('./gps_data.csv', 'w', newline='') as csvfile:
                self.writer = csv.writer(csvfile, delimiter='|')
                self.writer.writerow(['time_stamp', 'latitude', 'longitude', 'fix_quality', 'no_satellites', 'altitude'])
                #Timestamp format: MM/DD/YYYY hour:min:sec
        print('GPS initilialized. Ready to log data to gps_data.csv')


    def monitor_bus(self):
        while True:
            self.gps.update()
            
            if not self.gps.has_fix:
                # Try again if we don't have a fix yet.
                print("Waiting for fix...")
                continue

            with open('./gps_data.csv', 'a', newline='') as csvfile:
                writer = csv.writer(csvfile)
                writer.writerow(["{}/{}/{} {:02}:{:02}:{:02}".format(
                                        self.gps.timestamp_utc.tm_mon, 
                                        self.gps.timestamp_utc.tm_mday,  
                                        self.gps.timestamp_utc.tm_year,  
                                        self.gps.timestamp_utc.tm_hour,  
                                        self.gps.timestamp_utc.tm_min,  
                                        self.gps.timestamp_utc.tm_sec,
                                        ), 
                                        "{}".format(self.gps.latitude), 
                                        "{}".format(self.gps.longitude),
                                        "{}".format(self.gps.fix_quality), 
                                        "{}".format(self.gps.satellites),
                                        "{}".format(self.gps.altitude_m)
                                        ])
                print("Logged GPS data @{}/{}/{} {:02}:{:02}:{:02}".format(time.monotonic()))
                time.sleep(1) #sleep for 1 second

def main():
    monitor = GPSMonitor()
    monitor.monitor_bus()

if __name__ == "__main__":
    main()