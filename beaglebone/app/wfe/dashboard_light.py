from threading import Thread

from tkinter import Tk, Canvas, Entry, Text, Button, PhotoImage, Scale, scrolledtext
import time
import can
import cantools
import csv

# CAN arbitration ID constants
BATTERYSTATUSHV_ARB_ID = 2281967617
BMU_DTC_ARB_ID = 2147548929
MC_TEMP_ARB_ID = 2365563393
MC_TEMP_INV_ARB_ID = 2365562881

TEMPCOOLANT_L_ARB_ID = 2550137362
TEMPCOOLANT_R_ARB_ID = 2550137363

# declare water temp variable here because left/right
# sensor data is from diff CAN messages
water_temp_left = 0
water_temp_right = 0

BMU_VBATT_ARB_ID = 2281769985
WHEELSPEED_ARB_ID = 2287471618



window = Tk()

window.geometry("800x480")
window.configure(bg = "#262626")
window.title("UWFE Dashboard (LIGHT)")

canvas = Canvas(
    window,
    bg = "#262626",
    height = 480,
    width = 800,
    bd = 0,
    highlightthickness = 0,
    relief = "ridge"
)

canvas.place(x = 0, y = 0)

charge_bar = canvas.create_rectangle(
    1.0,
    2.0,
    801.0,
    58.00000000000006,
    fill="#FF0000",
    outline="")


soc_label = canvas.create_text(
    270.0,
    91.0,
    anchor="nw",
    text="SOC:",
    fill="#FFFFFF",
    font=("Lato Regular", 30 * -1)
)

soc_text = canvas.create_text(
    336.0,
    53.0,
    anchor="nw",
    text="100%",
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
    canvas.itemconfig(soc_text, text=f"{value}%")

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


temp_bg1 = canvas.create_rectangle(
    0.0,
    58.0,
    196.0,
    150.0,
    fill="#7125BD",
    outline="")

temp_bg2 = canvas.create_rectangle(
    0.0,
    323.0,
    196.0,
    415.0,
    fill="#0C15EA",
    outline="")

temp_bg3 = canvas.create_rectangle(
    0.0,
    234.0,
    196.0,
    326.0,
    fill="#BA007B",
    outline="")

temp_bg4 = canvas.create_rectangle(
    0.0,
    147.0,
    196.0,
    239.0,
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
    text="67.3°C",
    fill="#FFFFFF",
    font=("Lato Bold", 60 * -1)
)

motor_temp_text = canvas.create_text(
    7.0,
    163.0,
    anchor="nw",
    text="85.8°C",
    fill="#FFFFFF",
    font=("Lato Bold", 60 * -1)
)

inverter_temp_text = canvas.create_text(
    4.0,
    253.0,
    anchor="nw",
    text="79.9°C",
    fill="#FFFFFF",
    font=("Lato Bold", 60 * -1)
)

water_temp_text = canvas.create_text(
    4.0,
    340.0,
    anchor="nw",
    text="31.6°C",
    fill="#FFFFFF",
    font=("Lato Bold", 60 * -1)
)

deployment_label = canvas.create_text(
    235.0,
    139.0,
    anchor="nw",
    text="Deployment \n Last Lap:",
    fill="#FFFFFF",
    font=("Lato Bold", 20 * -1)
)

deployment_text = canvas.create_text(
    384.0,
    134.0,
    anchor="nw",
    text="4.8%",
    fill="#FFFFFF",
    font=("Lato Regular", 50 * -1)
)

speed_label = canvas.create_text(
    237.0,
    202.0,
    anchor="nw",
    text="Speed:",
    fill="#FFFFFF",
    font=("Lato Bold", 20 * -1)
)

speed_text = canvas.create_text(
    393.0,
    179.0,
    anchor="nw",
    text="34",
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
    651.0,
    77.0,
    anchor="nw",
    text="300V",
    fill="#FFFFFF",
    font=("Lato Bold", 40 * -1)
)

def changeMode():
    if(canvas.itemcget(mode_text, "text") == "RACE"):
        canvas.itemconfig(mode_text, text="SLOW")
    elif(canvas.itemcget(mode_text, "text") == "SLOW"):
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
    text="3.3V",
    fill="#FFFFFF",
    font=("Lato Bold", 40 * -1)
)

min_cell_text = canvas.create_text(
    621.0,
    278.0,
    anchor="nw",
    text="299V",
    fill="#FFFFFF",
    font=("Lato Bold", 40 * -1)
)

dtc_descriptions = {} 

with open('common/Data/DTC.csv', 'r') as file:
    reader = csv.reader(file)
    next(reader)  # Skip the header row
    for row in reader:
        dtc_code = int(row[0])
        description = row[6]
        dtc_descriptions[dtc_code] = description

db = cantools.database.load_file('common/Data/2024CAR.dbc')
can_bus = can.interface.Bus(channel='vcan0', bustype='socketcan')

 # Create the scrollable text area
dtc_text_area = scrolledtext.ScrolledText(window, width=45, height=2)
dtc_text_area.place(x=10, y=418)
dtc_text_area.config(font=("Helvetica", 20))
dtc_text_area.config(background='black')
dtc_text_area.config(foreground='red')

    # Function to update the text area with new error codes
def publish_dtc(error_code, error_message):
    description = dtc_descriptions.get(error_code, "Description not found")
    dtc_text_area.insert("end","\n" +str(error_code) + " | " + str(description))
    dtc_text_area.yview("end")  # Scroll to the bottom to show the latest message

def openDebug():
    window2 = Tk()
    window2.geometry("800x480")
    window2.configure(bg = "#262626")
    window2.title("UWFE Dashboard Debug")

    # Reset dashboard button
    reset_button = Button(window2, text="Reset Dashboard")
    reset_button.place(x=10, y=10)

    

    # Close Debug Menu button
    close_button = Button(window2, text="Close Debug Menu",command=window2.withdraw)
    close_button.place(x=220, y=10)

    # Create the scrollable text area
    debug_text_area = scrolledtext.ScrolledText(window2, width=100, height=30)
    debug_text_area.place(x=10, y=50)

    # Function to update the text area with new error codes
    def update_debug_text(error_code):
        debug_text_area.insert("end", error_code + "                                                " + time.strftime("%H:%M:%S") + "\n")
        debug_text_area.yview("end")  # Scroll to the bottom to show the latest message  

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
        print(message)

        if message is not None:
            print(message.arbitration_id)

        # Case for battery temp/soc
        if message.arbitration_id == BATTERYSTATUSHV_ARB_ID:
            print("message found!")
            try:
                decoded_data = db.decode_message(message.arbitration_id, message.data)

                soc_value = decoded_data['StateBatteryChargeHV']
                battery_temp = decoded_data['TempCellMax'] 

                update_battery_charge(soc_value)
                canvas.itemconfig(battery_temp_text, text=str(battery_temp))
            except KeyError:
                print("Unknown message ID")

        # Case for motor temp
        if message.arbitration_id == MC_TEMP_ARB_ID:
            try:
                decoded_data = db.decode_message(message.arbitration_id, message.data)

                motor_temp = decoded_data['INV_Motor_Temp']

                canvas.itemconfig(motor_temp_text, text=str(motor_temp))
            except KeyError:
                print("Unknown message ID")

        # Case for inverter temp
        if message.arbitration_id == MC_TEMP_INV_ARB_ID:
            try:
                decoded_data = db.decode_message(message.arbitration_id, message.data)

                inv_temp1 = decoded_data['INV_Module_A_Temp']
                inv_temp2 = decoded_data['INV_Module_B_Temp']
                inv_temp3 = decoded_data['INV_Module_C_Temp']

                average_inv_temp = (float(inv_temp1) + float(inv_temp2) + float(inv_temp3)) / 3

                canvas.itemconfig(inverter_temp_text, text=str(average_inv_temp))
            except KeyError:
                print("Unknown message ID")

        # Case for water temp (left)
        if message.arbitration_id == TEMPCOOLANT_L_ARB_ID:
            try:
                decoded_data = db.decode_message(message.arbitration_id, message.data)

                water_temp_left = decoded_data['TempInletRadMotorLeft']
                
                average_water_temp = (float(water_temp_left) + float(water_temp_right)) / 2

                canvas.itemconfig(water_temp_text, text=str(average_water_temp))
            except KeyError:
                print("Unknown message ID")
        
        # Case for water temp (right)
        if message.arbitration_id == TEMPCOOLANT_R_ARB_ID:
            try:
                decoded_data = db.decode_message(message.arbitration_id, message.data)

                water_temp_right = decoded_data['TempInletRadMotorRight']
                
                average_water_temp = (float(water_temp_left) + float(water_temp_right)) / 2

                canvas.itemconfig(water_temp_text, text=str(average_water_temp))
            except KeyError:
                print("Unknown message ID")

        # Case for VBATT
        if message.arbitration_id == BMU_VBATT_ARB_ID:
            try:
                decoded_data = db.decode_message(message.arbitration_id, message.data)

                vbatt = decoded_data['AMS_PackVoltage']

                canvas.itemconfig(vbatt_text, text=str(vbatt))
            except KeyError:
                print("Unknown message ID")

        # Case for Speeeeeed
        if message.arbitration_id == WHEELSPEED_ARB_ID:
            try:
                decoded_data = db.decode_message(message.arbitration_id, message.data)

                fl_speed = decoded_data['FLSpeedKPH']
                fr_speed = decoded_data['FRSpeedKPH']
                rr_speed = decoded_data['RRSpeedKPH']
                rl_speed = decoded_data['RLSpeedKPH']
                
                average_speed = (float(fl_speed) + float(fr_speed) + float(rr_speed) + float(rl_speed)) / 4

                canvas.itemconfig(speed_text, text=str(average_speed))
            except KeyError:
                print("Unknown message ID")

        # Case for BMU DTC
        if message.arbitration_id == BMU_DTC_ARB_ID:
            try:
                decoded_data = db.decode_message(message.arbitration_id, message.data)

                DTC_CODE = decoded_data['DTC_CODE']
                DTC_message = decoded_data['DTC_Data']   

                publish_dtc(DTC_CODE, DTC_message)
            except KeyError:
                print("Unknown message ID")
        

can_thread = Thread(target=process_can_messages)
can_thread.start()

window.resizable(False, False)
window.mainloop()
