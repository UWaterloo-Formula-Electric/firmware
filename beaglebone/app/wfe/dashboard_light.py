from threading import Thread
import math
from tkinter import Tk, Canvas, Entry, Text, Button, PhotoImage, Scale, scrolledtext
# sudo apt-get install python3-tk
# sudo pip3 install tk

import time
import traceback
import can
import cantools
import csv

CANBUS = 'can1'
db = cantools.db.load_file('/home/debian/firmware/common/Data/2024CAR.dbc')
can_bus = can.interface.Bus(channel=CANBUS, bustype='socketcan')

# CAN arbitration ID constants
BATTERYSTATUSHV_ARB_ID = db.get_message_by_name('BMU_batteryStatusHV').frame_id
HV_BUS_STATE_ARB_ID = db.get_message_by_name('BMU_stateBusHV').frame_id
BMU_DTC_ARB_ID = db.get_message_by_name('BMU_DTC').frame_id
MC_TEMP_ARB_ID = db.get_message_by_name('MC_Temperature_Set_3').frame_id
MC_TEMP_INV_ARB_ID = db.get_message_by_name('MC_Temperature_Set_1').frame_id

TEMPCOOLANT_L_ARB_ID = db.get_message_by_name('TempCoolantLeft').frame_id
TEMPCOOLANT_R_ARB_ID = db.get_message_by_name('TempCoolantRight').frame_id

LV_BATT_ARB_ID = db.get_message_by_name('PDU_batteryStatusLV').frame_id

# declare water temp variable here because left/right
# sensor data is from diff CAN messages
water_temp_left = 0
water_temp_right = 0

BMU_VBATT_ARB_ID = db.get_message_by_name('BMU_AmsVBatt').frame_id
WHEELSPEED_ARB_ID = db.get_message_by_name('WheelSpeedKPH').frame_id

window = Tk()

window.geometry("800x480")
window.configure(bg="#3f3f3f")
window.title("UWFE Dashboard (LIGHT)")

canvas = Canvas(
    window,
    bg="#262626",
    height=480,
    width=800,
    bd=0,
    highlightthickness=0,
    relief="ridge"
)

canvas.place(x=0, y=0)

charge_bar = canvas.create_rectangle(
    1.0,
    2.0,
    801.0,
    58.0,
    fill="#FF0000",
    outline="")


soc_label = canvas.create_text(
    240.0,
    90.0,
    anchor="nw",
    text="SOC:",
    fill="#FFFFFF",
    font=("Lato Regular", 30 * -1)
)

soc_text = canvas.create_text(
    330.0,
    50.0,
    anchor="nw",
    text="N/A",
    fill="#FFFFFF",
    font=("Lato Bold", 80 * -1)
)


def calculate_charge_color(charge):
    # Calculate a gradient from blue to red based on charge percentage
    r = int(255 * (1 - charge / 100))
    g = 0
    b = int(255 * (charge / 100))
    # Convert to hexadecimal color code
    color = "#{:02X}{:02X}{:02X}".format(r, g, b)
    return color


def update_battery_charge(value):

    # Convert value to an integer
    value = int(value)

    # Update the text
    canvas.itemconfig(soc_text, text='%.4s' % ('%.2f' % value) + '%')

    # Calculate the color based on the charge percentage
    charge_color = calculate_charge_color(value)

    # Update the charge bar color
    canvas.itemconfig(charge_bar, fill=charge_color)

    # Update the bar at the top
    canvas.coords(charge_bar, 1.0, 2.0, 1.0 + (value / 100) * 800, 58.0)


battery_charge_slider = Scale(
    window,
    from_=0,
    to=100,
    orient="horizontal",
    length=200,  # Adjust the length as needed
    sliderlength=20,
    showvalue=0,
    command=update_battery_charge
)
battery_charge_slider.place(x=370, y=300)  # Adjust the position as needed

# Initial configuration of the battery charge
update_battery_charge(battery_charge_slider.get())


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
    text="Battery Temp:",
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

battery_temp_text = canvas.create_text(
    6.0,
    76.0,
    anchor="nw",
    text="N/A",
    fill="#FFFFFF",
    font=("Lato Bold", 50 * -1)
)

motor_temp_text = canvas.create_text(
    7.0,
    163.0,
    anchor="nw",
    text="N/A",
    fill="#FFFFFF",
    font=("Lato Bold", 50 * -1)
)

inverter_temp_text = canvas.create_text(
    4.0,
    253.0,
    anchor="nw",
    text="N/A",
    fill="#FFFFFF",
    font=("Lato Bold", 50 * -1)
)

water_temp_text = canvas.create_text(
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

deployment_text = canvas.create_text(
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

speed_text = canvas.create_text(
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

vbatt_text = canvas.create_text(
    620.0,
    77.0,
    anchor="nw",
    text="N/A",
    fill="#FFFFFF",
    font=("Lato Bold", 40 * -1)
)


def changeMode():
    if (canvas.itemcget(mode_text, "text") == "RACE"):
        canvas.itemconfig(mode_text, text="SLOW")
    elif (canvas.itemcget(mode_text, "text") == "SLOW"):
        canvas.itemconfig(mode_text, text="RACE")


mode_button = Button(
    window,
    text="Mode",
    font=("Lato Regular", 15),
    bg="#4CAF50",
    fg="black",
    command=changeMode
)

# Position the button
mode_button.place(x=370, y=320)


mode_text = canvas.create_text(
    620.0,
    144.0,
    anchor="nw",
    text="RACE",
    fill="#FFFFFF",
    font=("Lato Bold", 40 * -1)
)

lv_batt_text = canvas.create_text(
    621.0,
    212.0,
    anchor="nw",
    text="N/A",
    fill="#FFFFFF",
    font=("Lato Bold", 40 * -1)
)

min_cell_text = canvas.create_text(
    621.0,
    278.0,
    anchor="nw",
    text="N/A",
    fill="#FFFFFF",
    font=("Lato Bold", 40 * -1)
)

dtc_descriptions = {}

with open('/home/debian/firmware/common/Data/DTC.csv', 'r') as file:
    reader = csv.reader(file)
    next(reader)  # Skip the header row
    for row in reader:
        dtc_code = int(row[0])
        description = row[6]
        dtc_descriptions[dtc_code] = description


# Create the scrollable text area
dtc_text_area = scrolledtext.ScrolledText(window, width=45, height=2)
dtc_text_area.place(x=10, y=418)
dtc_text_area.config(font=("Helvetica", 20))
dtc_text_area.config(background='black')
dtc_text_area.config(foreground='red')

# Function to update the text area with new error codes


def publish_dtc(error_code, error_message):
    description = dtc_descriptions.get(error_code, "Description not found")
    dtc_text_area.insert("end", "\n" + str(error_code) +
                         " | " + str(description))
    # Scroll to the bottom to show the latest message
    dtc_text_area.yview("end")


def openDebug():
    window2 = Tk()
    window2.geometry("800x480")
    window2.configure(bg="#262626")
    window2.title("UWFE Dashboard Debug")

    # Reset dashboard button
    reset_button = Button(window2, text="Reset Dashboard")
    reset_button.place(x=10, y=10)

    # Close Debug Menu button
    close_button = Button(window2, text="Close Debug Menu",
                          command=window2.withdraw)
    close_button.place(x=220, y=10)

    # Create the scrollable text area
    debug_text_area = scrolledtext.ScrolledText(window2, width=100, height=30)
    debug_text_area.place(x=10, y=50)

    # Function to update the text area with new error codes
    def update_debug_text(error_code):
        debug_text_area.insert(
            "end", error_code + "                                                " + time.strftime("%H:%M:%S") + "\n")
        # Scroll to the bottom to show the latest message
        debug_text_area.yview("end")

    # Function to simulate the stream of diagnostic codes (in a separate thread)
    def simulate_error_stream():
        error_count = 0
        while True:
            error_code = f"Error Code {error_count}"
            update_debug_text(error_code)
            error_count += 1
            time.sleep(0.1)  # Simulate error codes coming in every 2 seconds

    # Start the error code simulation thread
    error_thread = Thread(target=simulate_error_stream)
    error_thread.start()

    # pause debug stream button
    pause_button = Button(window2, text="Pause DTC Messages")
    pause_button.place(x=420, y=10)

    window2.resizable(False, False)
    window2.mainloop()


debug_button = Button(
    window,
    text="Debug",
    font=("Lato Regular", 15),
    bg="#4CAF50",
    fg="black",
    command=openDebug
)

debug_button.place(x=710, y=440)


def process_can_messages():
    print("reading can messages...")
    while True:
        message = can_bus.recv(timeout=0.1)
        if message is None:
            continue
        print(message)

        try:
            decoded_data = db.decode_message(
                message.arbitration_id, message.data)
            # Case for battery temp/soc
            if message.arbitration_id == BATTERYSTATUSHV_ARB_ID:
                soc_value = decoded_data['StateBatteryChargeHV']
                battery_temp = decoded_data['TempCellMax']

                update_battery_charge(soc_value)
                canvas.itemconfig(battery_temp_text, text='%.4s' %
                                  ('%.1f' % battery_temp) + '°C')

            # Case for motor temp
            if message.arbitration_id == MC_TEMP_ARB_ID:
                motor_temp = decoded_data['INV_Motor_Temp']
                coolant_temp = decoded_data['INV_Coolant_Temp']

                canvas.itemconfig(motor_temp_text, text='%.4s' %
                                  ('%.1f' % motor_temp) + '°C')
                canvas.itemconfig(water_temp_text, text='%.4s' %
                                  ('%.1f' % coolant_temp) + '°C')

            # Case for inverter temp
            if message.arbitration_id == MC_TEMP_INV_ARB_ID:
                inv_temp1 = decoded_data['INV_Module_A_Temp']
                inv_temp2 = decoded_data['INV_Module_B_Temp']
                inv_temp3 = decoded_data['INV_Module_C_Temp']

                average_inv_temp = (float(inv_temp1) +
                                    float(inv_temp2) + float(inv_temp3)) / 3

                canvas.itemconfig(inverter_temp_text,
                                  text='%.4s' % ('%.1f' % average_inv_temp) + '°C')
            # Case for VBATT
            if message.arbitration_id == BMU_VBATT_ARB_ID:
                vbatt = decoded_data['AMS_PackVoltage']

                canvas.itemconfig(vbatt_text, text='%.5s' %
                                  ('%.3f' % vbatt) + 'V')
            # Case for LV batt
            if message.arbitration_id == LV_BATT_ARB_ID:
                lvbatt = decoded_data['VoltageBusLV']
                canvas.itemconfig(lv_batt_text, text='%.5s' %
                                  ('%.3f' % lvbatt) + 'V')
            # Case for Min HV Cell voltage
            if message.arbitration_id == HV_BUS_STATE_ARB_ID:
                cell_min = decoded_data['VoltageCellMin']
                canvas.itemconfig(min_cell_text, text='%.4s' %
                                  ('%.3f' % cell_min) + 'V')
            # Case for Speeeeeed
            if message.arbitration_id == WHEELSPEED_ARB_ID:
                fl_speed = decoded_data['FLSpeedKPH']
                fr_speed = decoded_data['FRSpeedKPH']
                rr_speed = decoded_data['RRSpeedKPH']
                rl_speed = decoded_data['RLSpeedKPH']

                average_speed = (float(fl_speed) + float(fr_speed) +
                                 float(rr_speed) + float(rl_speed)) / 4

                canvas.itemconfig(speed_text, text='%.3s' %
                                  ('%.1f' % average_speed) + 'kph')

            # Case for BMU DTC
            if message.arbitration_id == BMU_DTC_ARB_ID:
                DTC_CODE = decoded_data['DTC_CODE']
                DTC_message = decoded_data['DTC_Data']

                publish_dtc(DTC_CODE, DTC_message)
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
            dtc_text_area.insert("end", "\n" + str(e) +
                         " | " + str(time.strftime("%H:%M:%S")))



can_thread = Thread(target=process_can_messages)
can_thread.start()

window.resizable(False, False)
window.mainloop()
