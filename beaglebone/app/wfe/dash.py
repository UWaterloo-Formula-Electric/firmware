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

WIDTH, HEIGHT = 800, 480


class Page(tk.Frame):
    def __init__(self, *args, **kwargs):
        tk.Frame.__init__(self, *args, **kwargs)

    def show(self):
        self.lift()


class DashPage(Page):
    def __init__(self, *args, **kwargs):
        Page.__init__(self, bg="#262626", *args, **kwargs)

        ### fonts ###
        self.title_font = "Lato Regular"
        self.text_font = "PT Mono"
        ncols = 3

        ### charge bar ###
        _charge_bar_height = 30
        _holder_frame = tk.Frame(self, bg=self["bg"], height=_charge_bar_height, width=WIDTH-2, highlightthickness=1)
        _holder_frame.grid(row=0, column=0, columnspan=ncols, sticky=tk.NW, ipady=1, padx=1)

        self.charge_bar = tk.Frame(_holder_frame, height=_charge_bar_height, width=0)
        self.charge_bar.place(x=0, y=0, anchor=tk.NW)

        ### Temps ###
        self.temp_cell_text = self._create_cell("#7125BD", "Max Cell Temp", row=1, col=0)
        self.temp_water_text = self._create_cell("#0C15EA", "Max Inv Temp", row=2, col=0)
        self.temp_inv_text = self._create_cell("#BA007B", "Max Water Temp", row=3, col=0)
        self.temp_motor_text = self._create_cell("#FF0101", "Max Motor Temp", row=4, col=0)

        ### CENTER ###
        self.soc_text = self._create_mid_cell("SOC:", row=1, col=1)
        self.speed_text = self._create_mid_cell("Speed:", row=2, col=1)
        self.deployment_text = self._create_mid_cell("Deployment:", row=3, col=1)

        ### UWFE ###
        self.uwfe_text = tk.Label(self, text="UWFE", font=("Helvetica", -50, "italic"), bg=self["bg"], fg="#afafaf")
        self.uwfe_text.grid(row=4, column=1, pady=1, padx=1)

        ### Voltages and mode ###
        self.vbatt_text = self._create_cell("#6B6B6B", "Pack Voltage", row=1, col=2)
        self.mode_text = self._create_cell("#6B6B6B", "Mode", row=2, col=2)
        self.lvbatt_text = self._create_cell("#6B6B6B", "LV Batt Voltage", row=3, col=2)
        self.min_cell_text = self._create_cell("#6B6B6B", "Min Cell Voltage", row=4, col=2)

        ### Debug ###
        # IMPORTANT: DO NOT CHANGE THE HEIGHT OF THE TEXT AREA, it will break the layout
        # this is cuz the height is acc num lines shown
        # and messing with the font or font size or lines will move stuff around
        self.dtc_text_area = tk.scrolledtext.ScrolledText(self, font=("Helvetica", -19),
                                                          width=50, height=6, bg="#000000", fg="#ff0000")
        self.dtc_text_area.grid(row=5, column=0, columnspan=ncols, sticky=tk.NSEW)
        self.dtc_text_area.config()
        # self.dtc_text_area.vbar.pack_forget() # remove scroll bar as we can use it on car's screen

        self.update_battery_charge(0)

        self._is_flash_enabled = False

    def _create_cell_frame(self, bg, row, col, sticky=tk.NSEW) -> tk.Frame:
        cell_frame = tk.Frame(self, bg=bg, height=74, highlightthickness=1, relief=tk.GROOVE)
        cell_frame.grid(row=row, column=col, sticky=sticky, pady=1, padx=1)
        return cell_frame

    def _create_cell(self, bg, title, row, col, sticky=tk.NSEW) -> tk.Label:
        cell_frame = self._create_cell_frame(bg, row, col, sticky)
        cell_title = tk.Label(cell_frame, text=title, bg=bg, fg="#ffffff", font=(self.title_font, -15))
        cell_title.place(anchor="nw")
        cell_text = tk.Label(cell_frame, text="N/A", bg=bg, fg="#ffffff", font=(self.text_font, -40))
        cell_text.place(x=10, y=20, anchor="nw")
        return cell_text

    def _create_mid_cell(self, title, row, col, sticky=tk.NSEW) -> tk.Label:
        cell_frame = self._create_cell_frame(self["bg"], row, col, sticky)
        cell_frame.config(width=200)
        cell_title = tk.Label(cell_frame, text=title, bg=self["bg"], fg="#ffffff", font=(self.title_font, -20))
        cell_title.place(x=10, rely=0.5, anchor="w")
        cell_text = tk.Label(cell_frame, text="N/A", bg=self["bg"], fg="#ffffff", font=(self.text_font, -53))
        cell_text.place(x=150, rely=0.5, anchor="w")
        return cell_text

    def update_battery_charge(self, value):
        def calculate_charge_color(charge):
            # Calculate a gradient from blue to red based on charge percentage
            r = int(255 * (1 - charge / 100))
            g = 0
            b = int(255 * (charge / 100))
            # Convert to hexadecimal color code
            color = "#{:02X}{:02X}{:02X}".format(r, g, b)
            return color

        # Convert value to an integer and clamp
        value = float(value)
        value = max(0, min(100, value))

        # Update the text
        self.soc_text.config(text='%.4s' % ('%.2f' % value) + '%')
        charge_color = calculate_charge_color(value)
        self.charge_bar.config(bg=charge_color, width=(value / 100) * WIDTH)

    def updateDtc(self, description, error_code):
        self. dtc_text_area.insert("end", "\n" + str(error_code) + " | " + str(description))
        # Scroll to the bottom to show the latest message
        self.dtc_text_area.yview("end")

    def updateSoc(self, decoded_data: dict):
        soc_value = decoded_data['StateBatteryChargeHV']
        battery_temp = decoded_data['TempCellMax']

        self.update_battery_charge(soc_value)
        self.temp_cell_text.config(text='%.4s' % ('%.1f' % battery_temp) + '°C')

    def updateMotorTemp(self, decoded_data: dict):
        motor_temp = decoded_data['INV_Motor_Temp']
        coolant_temp = decoded_data['INV_Coolant_Temp']

        self.temp_motor_text.config(text='%.4s' % ('%.1f' % motor_temp) + '°C')
        self.temp_water_text.config(text='%.4s' % ('%.1f' % coolant_temp) + '°C')

    def updateInverterTemp(self, decoded_data: dict):
        inv_temp1 = decoded_data['INV_Module_A_Temp']
        inv_temp2 = decoded_data['INV_Module_B_Temp']
        inv_temp3 = decoded_data['INV_Module_C_Temp']

        average_inv_temp = (float(inv_temp1) + float(inv_temp2) + float(inv_temp3)) / 3

        self.temp_inv_text.config(text='%.4s' % ('%.1f' % average_inv_temp) + '°C')

    def updateVBatt(self, decoded_data: dict):
        vbatt = decoded_data['AMS_PackVoltage']
        self.vbatt_text.config(text='%.5s' % ('%.3f' % vbatt) + 'V')

    def updateLVbatt(self, decoded_data: dict):
        # lvbatt is in mV, convert to V
        lvbatt = int(decoded_data['VoltageBusLV']) / 1000.
        self.lvbatt_text.config(text='%.6s' % ('%.5f' % lvbatt) + 'V')

    def updateMinCell(self, decoded_data: dict):
        cell_min = decoded_data['VoltageCellMin']
        self.min_cell_text.config(text='%.6s' % ('%.5f' % cell_min) + 'V')

    def updateSpeed(self, decoded_data: dict):
        fl_speed = decoded_data['FLSpeedKPH']
        fr_speed = decoded_data['FRSpeedKPH']
        rr_speed = decoded_data['RRSpeedKPH']
        rl_speed = decoded_data['RLSpeedKPH']

        average_speed = (float(fl_speed) + float(fr_speed) + float(rr_speed) + float(rl_speed)) / 4

        self.speed_text.config(text='%.4s' % ('%.2f' % average_speed) + 'kph')

    def enable_flash(self):
        self._is_flash_enabled = True
        self._flash_uwfe()

    def disable_flash(self):
        self._is_flash_enabled = False
        self.uwfe_text.config(bg=self["bg"], fg="#afafaf")

    def _flash_uwfe(self):
        if not self._is_flash_enabled:
            return
        bg = self.uwfe_text.cget("background")
        fg = self.uwfe_text.cget("foreground")
        self.uwfe_text.config(bg=fg, fg=bg)
        self.after(250, self._flash_uwfe)


class DebugPage(Page):
    def __init__(self, *args, **kwargs):
        Page.__init__(self, *args, **kwargs)
        reset_button = tk.Button(self, text="Reset Dashboard")
        reset_button.place(x=10, y=10)

        # Create the scrollable text area
        self.debug_text_area = scrolledtext.ScrolledText(self, width=100, height=30)
        self.debug_text_area.place(x=0, y=50)

        # pause debug stream button
        pause_button = tk.Button(self, text="Pause DTC Messages")
        pause_button.place(x=420, y=10)

    def update_debug_text(self, error_code, error_data):
        error_code = f"{str(error_code)}: {error_data}"
        self.debug_text_area.insert("end", f"{error_code.ljust(30, ' ')}{time.strftime('%H:%M:%S')}\n")
        # Scroll to the bottom to show the latest message
        self.debug_text_area.yview("end")


class MainView(tk.Frame):
    def __init__(self, *args, **kwargs):
        tk.Frame.__init__(self, *args, **kwargs)
        self.dashPage = DashPage(self)
        self.debugPage = DebugPage(self)

        container = tk.Frame(self)
        container.pack(side="top", fill="both", expand=True)

        self.dashPage.place(in_=container, x=0, y=0, relwidth=1, relheight=1)
        self.debugPage.place(in_=container, x=0, y=0, relwidth=1, relheight=1)

        self.dashPage.show()

    def update_debug_text(self, description, error_code, error_data):
        
        self.debugPage.update_debug_text(error_code, error_data)
        description = f"data: {error_data} | {description}"
        self.dashPage.updateDtc(description, error_code)


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
        self.BMU_DTC_ARB_ID = self.db.get_message_by_name('BMU_DTC').frame_id
        self.MC_TEMP_ARB_ID = self.db.get_message_by_name('MC_Temperature_Set_3').frame_id
        self.MC_TEMP_INV_ARB_ID = self.db.get_message_by_name('MC_Temperature_Set_1').frame_id

        self.TEMPCOOLANT_L_ARB_ID = self.db.get_message_by_name('TempCoolantLeft').frame_id
        self.TEMPCOOLANT_R_ARB_ID = self.db.get_message_by_name('TempCoolantRight').frame_id

        self.LV_BATT_ARB_ID = self.db.get_message_by_name('LV_Bus_Measurements').frame_id

        self.DCU_BUTTONS_ARB_ID = self.db.get_message_by_name('DCU_buttonEvents').frame_id

        self.BMU_VBATT_ARB_ID = self.db.get_message_by_name('BMU_AmsVBatt').frame_id
        self.WHEELSPEED_ARB_ID = self.db.get_message_by_name('WheelSpeedKPH').frame_id
        self.dtc_descriptions = {}
        self.load_dtc_descriptions()

    def load_dtc_descriptions(self):
        with open(self.home_dir / 'firmware/common/Data/DTC.csv', 'r') as file:
            reader = csv.reader(file)
            next(reader)  # Skip the header row
            for row in reader:
                dtc_code = int(row[0])
                description = row[6]
                self.dtc_descriptions[dtc_code] = description

    def publish_dtc(self, error_code, error_data):
        description = self.dtc_descriptions.get(error_code, "Description not found")
        self.main_view.update_debug_text(description, error_code, error_data)

    def process_can_messages(self):
        dashPage = self.main_view.dashPage
        print("reading can messages...")
        while True:
            message = self.can_bus.recv(timeout=None)
            print(message)
            try:
                decoded_data = self.db.decode_message(message.arbitration_id, message.data)
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

                # case for screen navigation button events
                if message.arbitration_id == self.DCU_BUTTONS_ARB_ID:
                    # Open debug menu if R button is pressed
                    # Close debug menu if L button is pressed
                    if decoded_data['ButtonScreenNavRightEnabled'] == 1:
                        self.main_view.debugPage.show()
                    if decoded_data['ButtonScreenNavLeftEnabled'] == 1:
                        self.main_view.dashPage.show()

                # Case for BMU DTC
                if message.arbitration_id == self.BMU_DTC_ARB_ID:
                    DTC_CODE = decoded_data['DTC_CODE']
                    DTC_message = decoded_data['DTC_Data']

                    self.publish_dtc(DTC_CODE, DTC_message)

            except (KeyError, cantools.database.errors.DecodeError):
                print(f"Message decode failed for {message.arbitration_id}")
            except Exception:
                ###########################################
                # I suspect the constant dash reload is due to an unhandled exception in the loop
                # The following will print to the dashboard directly in the DTC area
                # remove this once we figure out what the exceptions are
                ###########################################
                e = traceback.format_exc()
                print(e)
                # not a good way to do this. should not be calling the dash page directly
                self.main_view.dashPage.dtc_text_area.insert("end", "\n" + str(e) +
                                                             " | " + str(time.strftime("%H:%M:%S")))

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
