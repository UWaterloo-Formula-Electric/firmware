import tkinter as tk
from tkinter import scrolledtext

from page import Page
from constants import WIDTH, HEIGHT, TagEnum, RPM_TO_KPH, INTERLOCK_FAULT_CODES_DESC, InterlockFaultEnum


class DashPage(Page):
    def __init__(self, *args, **kwargs):
        bg = kwargs.pop("bg", "#262626")
        Page.__init__(self, bg=bg, *args, **kwargs)

        ### fonts ###
        self.title_font = "Lato Regular"
        self.text_font = "PT Mono"
        ncols = 3

        ### charge bar ###
        _charge_bar_height = 30
        # DONT CHANGE THE WIDTH OF THE HOLDER FRAME, it will break the layout
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
        self.il_text = self._create_mid_cell("Interlock State:", row=3, col=1)
        

        ### UWFE ###
        self.uwfe_text = tk.Label(self, text="UWFE", font=("Helvetica", -50, "italic"), bg=self["bg"], fg="#afafaf")
        self.uwfe_text.grid(row=4, column=1, pady=1, padx=1)

        ### Voltages and mode ###
        self.vbatt_text = self._create_cell("#6B6B6B", "Pack Voltage", row=1, col=2)
        self.power_text = self._create_cell("#6B6B6B", "HV Power", row=2, col=2)
        self.lvbatt_text = self._create_cell("#6B6B6B", "LV Batt Voltage", row=3, col=2)
        self.min_cell_text = self._create_cell("#6B6B6B", "Min Cell Voltage", row=4, col=2)

        ### Debug ###
        # IMPORTANT: DO NOT CHANGE THE HEIGHT OF THE TEXT AREA, it will break the layout
        # this is cuz the height is acc num lines shown
        # and messing with the font or font size or lines will move stuff around
        self.dtc_text_area = tk.scrolledtext.ScrolledText(self, font=("Helvetica", -14),
                                                          width=50, height=8, bg="#000000", fg="#e3e3e3")
        self.dtc_text_area.grid(row=5, column=0, columnspan=ncols, sticky=tk.NSEW)

        self.update_battery_charge(0)

        self._is_flash_enabled = False
        self.il_text.config(text=InterlockFaultEnum.PASSED.name)

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

    _update_dtc_first = True

    def updateDtc(self, dtc_origin, dtc_code, dtc_data, desc):
        if isinstance(dtc_code, int):
            dtc_code = f"{dtc_code:02d}"
        if isinstance(dtc_data, int):
            dtc_data = f"{dtc_data:03d}"

        if not self._update_dtc_first:
            dtc_origin = "\n" + dtc_origin

        self._update_dtc_first = False  # set to false after first update
        self.dtc_text_area.insert(tk.INSERT, dtc_origin, TagEnum.ORIGIN.value)
        self.dtc_text_area.insert(tk.INSERT, " | ")
        self.dtc_text_area.insert(tk.INSERT, dtc_code, TagEnum.CODE.value)
        self.dtc_text_area.insert(tk.INSERT, " | ")
        self.dtc_text_area.insert(tk.INSERT, dtc_data, TagEnum.DATA.value)
        self.dtc_text_area.insert(tk.INSERT, " | ")
        self.dtc_text_area.insert(tk.INSERT, desc, TagEnum.DESC.value)

        self.dtc_text_area.tag_config(TagEnum.ORIGIN.value, foreground="#70a2ff")
        self.dtc_text_area.tag_config(TagEnum.CODE.value, foreground="#ec70ff")
        self.dtc_text_area.tag_config(TagEnum.DATA.value, foreground="#ff6b6b")
        self.dtc_text_area.tag_config(TagEnum.DESC.value, foreground="#ffffff")

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

        max_inv_temp = max(float(inv_temp1), float(inv_temp2), float(inv_temp3))

        self.temp_inv_text.config(text='%.4s' % ('%.1f' % max_inv_temp) + '°C')

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
        rpm = decoded_data['INV_Motor_Speed']
        kph = rpm * RPM_TO_KPH
        self.speed_text.config(text='%.4s' % ('%.2f' % kph) + 'kph')

    def clearIL(self):
        self.il_text.config(text=INTERLOCK_FAULT_CODES_DESC[0])
        self.disable_flash()

    def updateIL(self, decoded_data: dict):
        il = decoded_data['BMU_checkFailed']
        fault = InterlockFaultEnum(il)
        if fault not in (InterlockFaultEnum.PASSED, InterlockFaultEnum.CBRB):
            self.uwfe_text.config(bg="#FF0101", fg="#000000")
            self.enable_flash()
        else:
            self.disable_flash()
        if fault != InterlockFaultEnum.CBRB:
            self.il_text.config(text=fault.name)
    
    def updatePower(self, decoded_data: dict):
        power = decoded_data['INV_Tractive_Power_kW']
        self.power_text.config(text='%.5s' % ('%.3f' % power) + 'kW')

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
        self.uwfe_text.after(250, self._flash_uwfe)
