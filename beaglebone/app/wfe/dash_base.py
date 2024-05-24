from cantools.typechecking import DecodeResultType
import tkinter as tk
from tkinter import scrolledtext
import time
from threading import Thread


class Page(tk.Frame):
    def __init__(self, *args, **kwargs):
        tk.Frame.__init__(self, *args, **kwargs)

    def show(self):
        self.lift()


class DashPage(Page):
    def __init__(self, *args, **kwargs):
        Page.__init__(self, *args, **kwargs)
        canvas = tk.Canvas(
            self,
            bg="#262626",
            height=480,
            width=800,
            bd=0,
            highlightthickness=0,
            relief="ridge"
        )
        self.canvas = canvas
        canvas.place(x=0, y=0)

        self.charge_bar = canvas.create_rectangle(0,0,800,60, fill="#0000FF", outline="")

        soc_label = canvas.create_text(
            240.0,
            90.0,
            anchor="nw",
            text="SOC:",
            fill="#FFFFFF",
            font=("Lato Regular", 30 * -1)
        )

        self.soc_text = canvas.create_text(
            330.0,
            50.0,
            anchor="nw",
            text="N/A",
            fill="#FFFFFF",
            font=("Lato Bold", 80 * -1)
        )

        battery_charge_slider = tk.Scale(
            self,
            from_=0,
            to=100,
            orient="horizontal",
            length=200,  # Adjust the length as needed
            sliderlength=20,
            showvalue=0,
            command=self.update_battery_charge
        )
        # Adjust the position as needed
        battery_charge_slider.place(x=370, y=300)

        # Initial configuration of the battery charge
        self.update_battery_charge(battery_charge_slider.get())

        temp_batt_bg = canvas.create_rectangle(
            0.0, 58.0, 196.0, 150.0,
            fill="#7125BD",
            outline="")

        temp_water_bg = canvas.create_rectangle(
            0.0, 323.0, 196.0, 415.0,
            fill="#0C15EA",
            outline="")

        temp_inv_bg = canvas.create_rectangle(
            0.0, 234.0, 196.0, 326.0,
            fill="#BA007B",
            outline="")

        temp_motor_bg = canvas.create_rectangle(
            0.0, 147.0, 196.0, 239.0,
            fill="#FF0101",
            outline="")

        battery_temp_label = canvas.create_text(
            4.0,
            62.0,
            anchor="nw",
            text="Max Cell Temp:",
            fill="#FFFFFF",
            font=("Lato Regular", 15 * -1)
        )

        motor_temp_label = canvas.create_text(
            4.0,
            150.0,
            anchor="nw",
            text="Motor Temp:",
            fill="#FFFFFF",
            font=("Lato Regular", 15 * -1)
        )

        inverter_temp_label = canvas.create_text(
            4.0,
            239.0,
            anchor="nw",
            text="Inverter Temp:",
            fill="#FFFFFF",
            font=("Lato Regular", 15 * -1)
        )

        water_temp_label = canvas.create_text(
            4.0,
            327.0,
            anchor="nw",
            text="Water Temp:",
            fill="#FFFFFF",
            font=("Lato Regular", 15 * -1)
        )

        self.battery_temp_text = canvas.create_text(
            6.0,
            76.0,
            anchor="nw",
            text="N/A",
            fill="#FFFFFF",
            font=("Lato Bold", 50 * -1)
        )

        self.motor_temp_text = canvas.create_text(
            7.0,
            163.0,
            anchor="nw",
            text="N/A",
            fill="#FFFFFF",
            font=("Lato Bold", 50 * -1)
        )

        self.inverter_temp_text = canvas.create_text(
            4.0,
            253.0,
            anchor="nw",
            text="N/A",
            fill="#FFFFFF",
            font=("Lato Bold", 50 * -1)
        )

        self.water_temp_text = canvas.create_text(
            4.0,
            340.0,
            anchor="nw",
            text="N/A",
            fill="#FFFFFF",
            font=("Lato Bold", 50 * -1)
        )

        deployment_label = canvas.create_text(
            240.0,
            150.0,
            anchor="nw",
            text="Deployment\nLast Lap:",
            fill="#FFFFFF",
            font=("Lato Bold", 20 * -1)
        )

        self.deployment_text = canvas.create_text(
            380.0,
            140.0,
            anchor="nw",
            text="N/A",
            fill="#FFFFFF",
            font=("Lato Regular", 50 * -1)
        )

        speed_label = canvas.create_text(
            240.0,
            220.0,
            anchor="nw",
            text="Speed:",
            fill="#FFFFFF",
            font=("Lato Bold", 20 * -1)
        )

        self.speed_text = canvas.create_text(
            380.0,
            210.0,
            anchor="nw",
            text="N/A",
            fill="#FFFFFF",
            font=("Lato Regular", 50 * -1)
        )

        border_rectangle1 = canvas.create_rectangle(
            603.0,
            58.0,
            799.0,
            128.0,
            fill="#6B6B6B",
            outline="#FFFFFF")

        border_rectangle2 = canvas.create_rectangle(
            603.0,
            126.0,
            799.0,
            196.0,
            fill="#6B6B6B",
            outline="#FFFFFF")

        border_rectangle3 = canvas.create_rectangle(
            603.0,
            193.0,
            799.0,
            263.0,
            fill="#6B6B6B",
            outline="#FFFFFF")

        border_rectangle4 = canvas.create_rectangle(
            603.0,
            261.0,
            799.0,
            331.0,
            fill="#6B6B6B",
            outline="#FFFFFF")

        vbatt_label = canvas.create_text(
            610.0,
            63.0,
            anchor="nw",
            text="V-Batt:",
            fill="#FFFFFF",
            font=("Lato Regular", 15 * -1)
        )

        mode_label = canvas.create_text(
            610.0,
            131.0,
            anchor="nw",
            text="Mode:",
            fill="#FFFFFF",
            font=("Lato Regular", 15 * -1)
        )

        lv_battery_label = canvas.create_text(
            610.0,
            197.0,
            anchor="nw",
            text="LV Battery Voltage:",
            fill="#FFFFFF",
            font=("Lato Regular", 15 * -1)
        )

        min_cell_label = canvas.create_text(
            610.0,
            265.0,
            anchor="nw",
            text="Min Cell Voltage:",
            fill="#FFFFFF",
            font=("Lato Regular", 15 * -1)
        )

        self.vbatt_text = canvas.create_text(
            620.0,
            77.0,
            anchor="nw",
            text="N/A",
            fill="#FFFFFF",
            font=("Lato Bold", 40 * -1)
        )

        self.mode_text = canvas.create_text(
            620.0,
            144.0,
            anchor="nw",
            text="RACE",
            fill="#FFFFFF",
            font=("Lato Bold", 40 * -1)
        )

        self.lv_batt_text = canvas.create_text(
            621.0,
            212.0,
            anchor="nw",
            text="N/A",
            fill="#FFFFFF",
            font=("Lato Bold", 40 * -1)
        )

        self.min_cell_text = canvas.create_text(
            621.0,
            278.0,
            anchor="nw",
            text="N/A",
            fill="#FFFFFF",
            font=("Lato Bold", 40 * -1)
        )

        # Create the scrollable text area
        self.dtc_text_area = scrolledtext.ScrolledText(
            self, width=45, height=6)
        self.dtc_text_area.place(x=220, y=300)
        self.dtc_text_area.config(font=("Helvetica", 10))
        self.dtc_text_area.config(background='black')
        self.dtc_text_area.config(foreground='red')
    
    def updateDtc(self, description, error_code):
        self. dtc_text_area.insert("end", "\n" + str(error_code) +
                                   " | " + str(description))
        # Scroll to the bottom to show the latest message
        self.dtc_text_area.yview("end")

    def calculate_charge_color(self, charge):
        # Calculate a gradient from blue to red based on charge percentage
        r = int(255 * (1 - charge / 100))
        g = 0
        b = int(255 * (charge / 100))
        # Convert to hexadecimal color code
        color = "#{:02X}{:02X}{:02X}".format(r, g, b)
        return color

    def update_battery_charge(self, value):

        # Convert value to an integer
        value = int(value)

        # Update the text
        self.canvas.itemconfig(self.soc_text, text='%.4s' %
                               ('%.2f' % value) + '%')

        # Calculate the color based on the charge percentage
        charge_color = self.calculate_charge_color(value)

        # Update the charge bar color
        self.canvas.itemconfig(self.charge_bar, fill=charge_color)

        # Update the bar at the top
        self.canvas.coords(self.charge_bar, 0, 0, (value / 100) * 800, 60)

    def changeMode(self):
        if (self.canvas.itemcget(self.mode_text, "text") == "RACE"):
            self.canvas.itemconfig(self.mode_text, text="SLOW")
        elif (self.canvas.itemcget(self.mode_text, "text") == "SLOW"):
            self.canvas.itemconfig(self.mode_text, text="RACE")

    def updateSoc(self, decoded_data: DecodeResultType):
        soc_value = decoded_data['StateBatteryChargeHV']
        battery_temp = decoded_data['TempCellMax']

        self.update_battery_charge(soc_value)
        self.canvas.itemconfig(self.battery_temp_text, text='%.4s' %
                               ('%.1f' % battery_temp) + '°C')

    def updateMotorTemp(self, decoded_data: DecodeResultType):
        motor_temp = decoded_data['INV_Motor_Temp']
        coolant_temp = decoded_data['INV_Coolant_Temp']

        self.canvas.itemconfig(self.motor_temp_text, text='%.4s' %
                               ('%.1f' % motor_temp) + '°C')
        self.canvas.itemconfig(self.water_temp_text, text='%.4s' %
                               ('%.1f' % coolant_temp) + '°C')

    def updateInverterTemp(self, decoded_data: DecodeResultType):
        inv_temp1 = decoded_data['INV_Module_A_Temp']
        inv_temp2 = decoded_data['INV_Module_B_Temp']
        inv_temp3 = decoded_data['INV_Module_C_Temp']

        average_inv_temp = (float(inv_temp1) +
                            float(inv_temp2) + float(inv_temp3)) / 3

        self.canvas.itemconfig(self.inverter_temp_text,
                               text='%.4s' % ('%.1f' % average_inv_temp) + '°C')

    def updateVBatt(self, decoded_data: DecodeResultType):
        vbatt = decoded_data['AMS_PackVoltage']

        self.canvas.itemconfig(self.vbatt_text, text='%.5s' %
                               ('%.3f' % vbatt) + 'V')

    def updateLVbatt(self, decoded_data: DecodeResultType):
        lvbatt = decoded_data['VoltageBusLV']
        self.canvas.itemconfig(self.lv_batt_text, text='%.5s' %
                               ('%.3f' % lvbatt) + 'V')

    def updateMinCell(self, decoded_data: DecodeResultType):
        cell_min = decoded_data['VoltageCellMin']
        self.canvas.itemconfig(self.min_cell_text, text='%.4s' %
                               ('%.3f' % cell_min) + 'V')

    def updateSpeed(self, decoded_data: DecodeResultType):
        fl_speed = decoded_data['FLSpeedKPH']
        fr_speed = decoded_data['FRSpeedKPH']
        rr_speed = decoded_data['RRSpeedKPH']
        rl_speed = decoded_data['RLSpeedKPH']

        average_speed = (float(fl_speed) + float(fr_speed) +
                         float(rr_speed) + float(rl_speed)) / 4

        self.canvas.itemconfig(self.speed_text, text='%.3s' %
                               ('%.1f' % average_speed) + 'kph')

class DebugPage(Page):
    def __init__(self, *args, **kwargs):
        Page.__init__(self, *args, **kwargs)
        tk.Label(self, text="Page 2", font=("Helvetica", 16), fg="#ffffff", background="#3f3fff").pack(side="top", fill="both", expand=True)

class MainView(tk.Frame):
    def __init__(self, *args, **kwargs):
        tk.Frame.__init__(self, *args, **kwargs)
        self.dashPage = DashPage(self)
        self.debugPage = DebugPage(self)

        container = tk.Frame(self)
        container.pack(side="top", fill="both", expand=True)
        
        tk.Label(container, text="MainView", font=("Arial", 24)).place(relx=0.5, rely=0.5, anchor="center")

        self.dashPage.place(in_=container, x=0, y=0, relwidth=1, relheight=1)
        self.debugPage.place(in_=container, x=0, y=0, relwidth=1, relheight=1)




def switch_thread():
    while True:
        time.sleep(1)
        print("Switching to DebugPage")
        main.debugPage.show()
        time.sleep(1)
        print("Switching to DashPage")
        main.dashPage.show()


if __name__ == "__main__":
    root = tk.Tk()
    main = MainView(root)
    main.place(x=0, y=0, relwidth=1, relheight=1)
    root.wm_geometry("800x480")
    root.configure(bg="#3f3f3f")
    root.title("UWFE Dashboard (LIGHT)")
    Thread(target=switch_thread, daemon=True).start()
    root.mainloop()