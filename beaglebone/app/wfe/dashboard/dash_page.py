import tkinter as tk
from tkinter import scrolledtext, messagebox

from page import Page
from constants import WIDTH, HEIGHT, TagEnum, RPM_TO_KPH


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

        ### Temps ### Change colors here ->
        self.temp_cell_text = self._create_cell("#7125BD", "Max Cell Temp", row=1, col=0)
        self.temp_inv_text = self._create_cell("#0C15EA", "Max Inv Temp", row=2, col=0)
        self.temp_water_text = self._create_cell("#BA007B", "Max Water Temp", row=3, col=0)
        self.temp_motor_text = self._create_cell("#FF0101", "Max Motor Temp", row=4, col=0)

        ### CENTER ###
        self.soc_text = self._create_mid_cell("SOC:", row=1, col=1)
        self.speed_text = self._create_mid_cell("Speed:", row=2, col=1)
        self.deployment_text = self._create_mid_cell("Deployment:", row=3, col=1)

        ### UWFE ###
        self.uwfe_text = tk.Label(self, text="UWFE", font=("Helvetica", -50, "italic"), bg=self["bg"], fg="#afafaf")
        self.uwfe_text.grid(row=4, column=1, pady=1, padx=1)

        ### Voltages and mode ###
        self.vbatt_text = self._create_cell("#6B6B6B", "AMS Pack Voltage", row=1, col=2)
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
        #todo: cell voltage 2.5-4.2V, get tag for specific cell that is high temp/voltage
        self.temp_targets = {
            "cell": {"target": 25, "range": 30, "max_temp": 55},
            "water": {"target": 25, "range": 35, "max_temp": 60},
            "inv": {"target": 25, "range": 55, "max_temp": 80},
            "motor": {"target": 25, "range": 55, "max_temp": 80},
            "voltage": {"target": 2.5, "range": 1.7, "max_temp": 4.2}
        }
        
    def _calculate_temp_colour(self, temp, sensor_type):
        if temp is None:
            return "#808080"  # Grey for N/A
        
        target = self.temp_targets[sensor_type]["target"]
        temp_range = self.temp_targets[sensor_type]["range"]
        
        diff = temp - target
        if diff <= 0:
            return "#0000FF"  # Blue for cold or at target
        
        if diff > temp_range:
            return "#FF0000"  # Red for very hot
        
        # Calculate gradient from blue to red
        ratio = diff / temp_range
        r = int(255 * ratio)
        b = int(255 * (1 - ratio))
        return f"#{r:02x}00{b:02x}"

    def _check_temp_warning(self, temp, sensor_type):
        max_temp = self.temp_targets[sensor_type]["max_temp"]
        if temp >= max_temp - 5:
            warning_box = tk.Label(self, text="COOL THE CAR", font=("Helvetica", -40, "bold"), bg=self["bg"], fg="#FF0000", highlightbackground="#FF0000", highlightthickness=2)
            warning_box.place(relx=0.5, rely=0.63, anchor="center")
            self.after(2000, warning_box.destroy)

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

        battery_colour = self._calculate_temp_colour(battery_temp, "cell")
        cell_frame = self.temp_cell_text.master
        cell_frame.config(bg=battery_colour)
        for widget in cell_frame.winfo_children():
            widget.config(bg=battery_colour)

        self._check_temp_warning(battery_temp, "cell")

    def updateMotorTemp(self, decoded_data: dict):
        motor_temp = decoded_data['INV_Motor_Temp']
        coolant_temp = decoded_data['INV_Coolant_Temp']

        self.temp_motor_text.config(text='%.4s' % ('%.1f' % motor_temp) + '°C')
        self.temp_water_text.config(text='%.4s' % ('%.1f' % coolant_temp) + '°C')

        motor_colour = self._calculate_temp_colour(motor_temp, "motor")
        cell_frame = self.temp_motor_text.master
        cell_frame.config(bg=motor_colour)
        for widget in cell_frame.winfo_children():
            widget.config(bg=motor_colour)
        
        water_colour = self._calculate_temp_colour(coolant_temp, "water")
        cell_frame = self.temp_water_text.master
        cell_frame.config(bg=water_colour)
        for widget in cell_frame.winfo_children():
            widget.config(bg=water_colour)

        self._check_temp_warning(motor_temp, "motor")
        self._check_temp_warning(coolant_temp, "water")

    def updateInverterTemp(self, decoded_data: dict):
        inv_temp1 = decoded_data['INV_Module_A_Temp']
        inv_temp2 = decoded_data['INV_Module_B_Temp']
        inv_temp3 = decoded_data['INV_Module_C_Temp']

        max_inv_temp = max(float(inv_temp1), float(inv_temp2), float(inv_temp3))

        self.temp_inv_text.config(text='%.4s' % ('%.1f' % max_inv_temp) + '°C')

        inv_colour = self._calculate_temp_colour(max_inv_temp, "inv")
        cell_frame = self.temp_inv_text.master
        cell_frame.config(bg=inv_colour)
        for widget in cell_frame.winfo_children():
            widget.config(bg=inv_colour)

        self._check_temp_warning(max_inv_temp, "inv")

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

        voltage_colour = self._calculate_temp_colour(cell_min, "voltage")
        cell_frame = self.min_cell_text.master
        cell_frame.config(bg=voltage_colour)
        for widget in cell_frame.winfo_children():
            widget.config(bg=voltage_colour)

    def updateSpeed(self, decoded_data: dict):
        rpm = decoded_data['INV_Motor_Speed']
        kph = rpm * RPM_TO_KPH
        self.speed_text.config(text='%.4s' % ('%.2f' % kph) + 'kph')
    
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
