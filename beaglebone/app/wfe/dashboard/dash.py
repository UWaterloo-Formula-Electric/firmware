# pylint: disable=missing-function-docstring, missing-module-docstring, missing-class-docstring
import os
from threading import Thread
import tkinter as tk
from tkinter import scrolledtext
from pathlib import Path

import time
import traceback
import can
import cantools
import csv

from dash_page import DashPage
from debug_page import DebugPage
from constants import WIDTH, HEIGHT, INV_FAULT_CODES_DESC


class MainView(tk.Frame):
    def __init__(self, *args, **kwargs):
        tk.Frame.__init__(self, *args, **kwargs)
        bg = "#262626"
        self.dashPage = DashPage(self, bg=bg)
        self.debugPage = DebugPage(self, bg=bg)

        container = tk.Frame(self)
        container.pack(side="top", fill="both", expand=True)

        self.dashPage.place(in_=container, x=0, y=0, relwidth=1, relheight=1)
        self.debugPage.place(in_=container, x=0, y=0, relwidth=1, relheight=1)

        self.dashPage.show()

    def update_debug_text(self, dtc_origin, dtc_code, dtc_data, dtc_desc):
        self.debugPage.update_debug_text(dtc_origin, dtc_code, dtc_data, dtc_desc)
        self.dashPage.updateDtc(dtc_origin, dtc_code, dtc_data, dtc_desc)

    def scroll_debug_text(self, scroll_amount: int):
        self.dashPage.dtc_text_area.yview_scroll(scroll_amount, "units")
        self.debugPage.debug_text_area.yview_scroll(scroll_amount, "units")
    
    def update_motor_fault(self, decoded_data):
        inv_post_fault_hi = decoded_data['INV_Post_Fault_Hi']
        inv_post_fault_lo = decoded_data['INV_Post_Fault_Lo']
        inv_run_fault_hi = decoded_data['INV_Run_Fault_Hi']
        inv_run_fault_lo = decoded_data['INV_Run_Fault_Lo']
        
        inv_post_fault = (inv_post_fault_hi << 15) | inv_post_fault_lo
        inv_run_fault = (inv_run_fault_hi << 15) | inv_run_fault_lo

        inv_fault = (inv_run_fault << 32)  | inv_post_fault

        if inv_fault == 0:
            return
        
        description = INV_FAULT_CODES_DESC.get(inv_fault, "Unknown Inverter Fault")

        if inv_post_fault != 0: 
            fault_type = "Post Fault"
            dtc_code = inv_post_fault
        else:
            fault_type = "Run Fault"
            dtc_code = inv_run_fault

        self.dashPage.updateDtc(dtc_origin="INV", dtc_code=f"0x{dtc_code:08x}", dtc_data=fault_type, desc=description)
        self.debugPage.update_debug_text(dtc_origin="INV", dtc_code=f"0x{dtc_code:08x}", dtc_data=fault_type, dtc_desc=description)

        


class CANProcessor:
    def __init__(self, main_view: MainView):
        self.main_view = main_view

        # Messed up too many times with the CANBUS variable
        # just detect user and set the CANBUS variable
        if os.getlogin() == 'vagrant':
            CANBUS = 'vcan0'
            self.home_dir = Path("/home/vagrant/shared")
        else:
            CANBUS = 'can1'
            self.home_dir = Path("/home/debian")

        self.db = cantools.db.load_file(self.home_dir / 'firmware/common/Data/2024CAR.dbc')
        self.can_bus = can.interface.Bus(channel=CANBUS, bustype='socketcan')

        # CAN arbitration ID constants
        self.BATTERYSTATUSHV_ARB_ID = self.db.get_message_by_name('BMU_batteryStatusHV').frame_id
        self.HV_BUS_STATE_ARB_ID = self.db.get_message_by_name('BMU_stateBusHV').frame_id

        self.MC_TEMP_ARB_ID = self.db.get_message_by_name('MC_Temperature_Set_3').frame_id
        self.MC_TEMP_INV_ARB_ID = self.db.get_message_by_name('MC_Temperature_Set_1').frame_id

        self.TEMPCOOLANT_L_ARB_ID = self.db.get_message_by_name('TempCoolantLeft').frame_id
        self.TEMPCOOLANT_R_ARB_ID = self.db.get_message_by_name('TempCoolantRight').frame_id

        self.LV_BATT_ARB_ID = self.db.get_message_by_name('LV_Bus_Measurements').frame_id

        self.DCU_BUTTONS_ARB_ID = self.db.get_message_by_name('DCU_buttonEvents').frame_id

        self.BMU_VBATT_ARB_ID = self.db.get_message_by_name('BMU_AmsVBatt').frame_id
        self.WHEELSPEED_ARB_ID = self.db.get_message_by_name('WheelSpeedKPH').frame_id

        self.MC_FAULT_CODES_ARB_ID = self.db.get_message_by_name('MC_Fault_Codes').frame_id

        self.DTC_ARB_IDS = [msg.frame_id for msg in self.db.messages if 'DTC' in msg.name]
        self.dtcs = {}
        self.load_dtcs()

    def load_dtcs(self):
        """
        Load the DTC data from DTC.csv into a dictionary with format

        dtcs = {
            dtc_code: {
                name: "CELL_TEMP_LOW",
                origin: "BMU",
                severity: 1,
                subscribers: "VCU, PDU",
                data: 1231,
                description: "desc"
                },
            ...
        }
        """
        with open(self.home_dir / 'firmware/common/Data/DTC.csv', 'r') as file:
            reader = csv.reader(file)
            next(reader)  # Skip the header row
            for row in reader:
                dtc_code = int(row[0])
                self.dtcs[dtc_code] = {
                    'name': row[1],
                    'origin': row[2],
                    'severity': row[3],
                    'subscribers': row[4],
                    'data': row[5],
                    'description': row[6],
                }
        return self.dtcs

    def parse_dtc_description(self, description: str, dtc_data: int) -> str:
        """
        Parse the DTC description string to replace reasons with data from the DTC

        Expected format in desc
        case 1: Some error caused by #data {0: "reason1", 1: "reason2", ...}
            #data will be replaced by the reason corresponding to the DTC_data
        case 2: Some error caused by #data
            #data will be replaced by the DTC_data
        """
        desc_split = description.split('{')
        reasons = {}
        replacement = str(dtc_data)
        if len(desc_split) > 1:
            reasons = eval("{" + desc_split[1])
            try:
                replacement = reasons[dtc_data]
            except KeyError:
                replacement = f"Unkown reason: {dtc_data}"
        return desc_split[0].replace('#data', replacement)

    def format_dtc(self, dtc_origin: str, dtc_code: int, dtc_data: int) -> list:
        """
        Format the DTC code and data into a human-readable string with description from DTC.csv
        """
        description = self.dtcs.get(dtc_code, {"description": f"DTC Code: {dtc_code} not found"})["description"]
        description = self.parse_dtc_description(description, dtc_data)
        if dtc_origin.lower().startswith("charge"):
            dtc_origin = "CCU"
        if dtc_origin.lower().startswith("vcu"):
            dtc_origin = "VCU"
        dtc_origin = dtc_origin.replace("_DTC", "")
        return dtc_origin, dtc_code, dtc_data, description

    def publish_dtc(self, dtc_origin, dtc_code, dtc_data):
        dtc_origin, dtc_code, dtc_data, dtc_desc = self.format_dtc(dtc_origin, dtc_code, dtc_data)
        self.main_view.update_debug_text(dtc_origin, dtc_code, dtc_data, dtc_desc)

    def process_can_messages(self):
        dashPage = self.main_view.dashPage
        _last_scr_btn_ts = time.time()
        _last_scr_btn = None

        print("reading can messages...")
        while True:
            
            if _last_scr_btn is not None and time.time() - _last_scr_btn_ts > 1:
                print("out " + _last_scr_btn)
                if _last_scr_btn == "R":
                    self.main_view.debugPage.show()
                if _last_scr_btn == "L":
                    self.main_view.dashPage.show()
                _last_scr_btn = None
            message = self.can_bus.recv(timeout=0.1)
            try:
                if message is None:
                    continue
                decoded_data = self.db.decode_message(message.arbitration_id, message.data)
            except KeyError:
                print(f"Missing key {message.arbitration_id} in message decode")
                continue
            except cantools.database.errors.DecodeError:
                print(f"Message decode failed for {message.arbitration_id}")
                continue
            
            print(message)
            try:
                # Case for battery temp/soc
                if message.arbitration_id == self.BATTERYSTATUSHV_ARB_ID:
                    dashPage.updateSoc(decoded_data)
                    # Case for motor temp
                if message.arbitration_id == self.MC_TEMP_ARB_ID:
                    dashPage.updateMotorTemp(decoded_data)

                # Case for inverter temp
                if message.arbitration_id == self.MC_TEMP_INV_ARB_ID:
                    dashPage.updateInverterTemp(decoded_data)
                    # Case for VBATT
                if message.arbitration_id == self.BMU_VBATT_ARB_ID:
                    dashPage.updateVBatt(decoded_data)

                # Case for LV batt
                if message.arbitration_id == self.LV_BATT_ARB_ID:
                    dashPage.updateLVbatt(decoded_data)

                # Case for Min HV Cell voltage
                if message.arbitration_id == self.HV_BUS_STATE_ARB_ID:
                    dashPage.updateMinCell(decoded_data)
                # Case for Speeeeeed
                if message.arbitration_id == self.WHEELSPEED_ARB_ID:
                    dashPage.updateSpeed(decoded_data)
                if message.arbitration_id == self.MC_FAULT_CODES_ARB_ID:
                    self.main_view.update_motor_fault(decoded_data)
                # case for screen navigation button events
                if message.arbitration_id == self.DCU_BUTTONS_ARB_ID:
                    # scroll up if R button double clicked
                    # scroll down if L button double
                    # Open debug menu if R button is pressed
                    # Close debug menu if L button is pressed
                    
                    t1 = time.time()
                    r_btn = decoded_data['ButtonScreenNavRightEnabled'] == 1
                    l_btn = decoded_data['ButtonScreenNavLeftEnabled'] == 1
                    scrolled = False
  
                    if t1 - _last_scr_btn_ts < 1:
                        if r_btn and _last_scr_btn == "R":
                            self.main_view.scroll_debug_text(-5)
                            scrolled = True
                            
                        if l_btn and _last_scr_btn == "L":
                                self.main_view.scroll_debug_text(5)
                                scrolled = True

                    if r_btn:                        
                        _last_scr_btn = "R"
                    if l_btn:
                        _last_scr_btn = "L"
                    
                    if scrolled:
                        _last_scr_btn = None

                    _last_scr_btn_ts = t1
                
                # Case for BMU DTC
                if message.arbitration_id in self.DTC_ARB_IDS:
                    dtc_origin = self.db.get_message_by_frame_id(message.arbitration_id).name
                    dtc_code = decoded_data['DTC_CODE']
                    dtc_data = decoded_data['DTC_Data']

                    self.publish_dtc(dtc_origin, dtc_code, dtc_data)

            except Exception:
                ###########################################
                # I suspect the constant dash reload is due to an unhandled exception in the loop
                # The following will print to the dashboard directly in the DTC area
                # remove this once we figure out what the exceptions are
                ###########################################
                e = traceback.format_exc()
                print(e)
                # not a good way to do this. should not be calling the dash page directly
                self.main_view.debugPage.debug_text_area.insert("end", "\n" + str(e) +
                                                             " | " + str(time.strftime("%H:%M:%S")))

    def buttons_thread(self):
        pass
    def start_can_thread(self):
        # enable daemon to kill the thread when the main thread exits)
        Thread(target=self.process_can_messages, daemon=True).start()


if __name__ == "__main__":
    root = tk.Tk()
    main = MainView(root)
    main.place(x=0, y=0, relwidth=1, relheight=1)
    root.wm_geometry(f"{WIDTH}x{HEIGHT}")
    root.title("UWFE Dashboard (LIGHT)")
    CANProcessor(main).start_can_thread()
    root.mainloop()
