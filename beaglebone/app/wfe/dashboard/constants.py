import enum
from math import pi
WIDTH = 800
HEIGHT = 480

BUTTON_SCROLL_TIMEOUT_S = 0.3

WHEEL_DIAMETER_M = 16 * 2.54 / 100
GEAR_RATIO_MOT_TO_WHEEL = 3.75
WHEEL_CIRCUMFRENCE = WHEEL_DIAMETER_M * pi
M_TO_KM = 1000
MIN_PER_HR = 60

RPM_TO_KPH = (WHEEL_CIRCUMFRENCE / GEAR_RATIO_MOT_TO_WHEEL) / M_TO_KM * MIN_PER_HR 


class TagEnum(enum.Enum):
    ORIGIN = "origin"
    CODE = "code"
    DATA = "data"
    DESC = "desc"
    TIME = "time"


INV_FAULT_CODES_DESC = {
    1 << 0: "Hardware Gate/Desaturation Fault",
    1 << 1: "HW Over-current Fault",
    1 << 2: "Accelerator Shorted",
    1 << 3: "Accelerator Open",
    1 << 4: "Current Sensor Low",
    1 << 5: "Current Sensor High",
    1 << 6: "Module Temperature Low",
    1 << 7: "Module Temperature High",
    1 << 8: "Control PCB Temperature Low",
    1 << 9: "Control PCB Temperature High",
    1 << 10: "Gate Drive PCB Temperature Low",
    1 << 11: "Gate Drive PCB Temperature High",
    1 << 12: "5V Sense Voltage Low",
    1 << 13: "5V Sense Voltage High",
    1 << 14: "12V Sense Voltage Low",
    1 << 15: "12V Sense Voltage High",
    1 << 16: "2.5V Sense Voltage Low",
    1 << 17: "2.5V Sense Voltage High",
    1 << 18: "1.5V Sense Voltage Low",
    1 << 19: "1.5V Sense Voltage High",
    1 << 20: "DC Bus Voltage High",
    1 << 21: "DC Bus Voltage Low",
    1 << 22: "Pre-charge Timeout",
    1 << 23: "Pre-charge Voltage Failure",
    1 << 24: "EEPROM Checksum Invalid",
    1 << 25: "EEPROM Data Out of Range",
    1 << 26: "EEPROM Update Required",
    1 << 27: "Hardware DC Bus Over-Voltage during initialization",
    1 << 28: "Gen 3: Reserved, Gen 5: Gate Driver Initialization",
    1 << 29: "Reserved",
    1 << 30: "Brake Shorted",
    1 << 31: "Brake Open",
    1 << 32: "Motor Over-speed Fault",
    1 << 33: "Over-current Fault",
    1 << 34: "Over-voltage Fault",
    1 << 35: "Inverter Over-temperature Fault",
    1 << 36: "Accelerator Input Shorted Fault",
    1 << 37: "Accelerator Input Open Fault",
    1 << 38: "Direction Command Fault",
    1 << 39: "Inverter Response Time-out Fault",
    1 << 40: "Hardware Gate/Desaturation Fault",
    1 << 41: "Hardware Over-current Fault",
    1 << 42: "Under-voltage Fault",
    1 << 43: "CAN Command Message Lost Fault",
    1 << 44: "Motor Over-temperature Fault",
    1 << 45: "Reserved",
    1 << 46: "Reserved",
    1 << 47: "Reserved",
    1 << 48: "Brake Input Shorted Fault",
    1 << 49: "Brake Input Open Fault",
    1 << 50: "Module A Over-temperature Fault",
    1 << 51: "Module B Over-temperature Fault",
    1 << 52: "Module C Over-temperature Fault",
    1 << 53: "PCB Over-temperature Fault7",
    1 << 54: "Gate Drive Board 1 Over-temperature Fault",
    1 << 55: "Gate Drive Board 2 Over-temperature Fault",
    1 << 56: "Gate Drive Board 3 Over-temperature Fault",
    1 << 57: "Current Sensor Fault",
    1 << 58: "Gen 3: Reserved, Gen 5: Gate Driver Over-Voltage",
    1 << 59: "Gen 3: Hardware DC Bus Over-Voltage Fault, Gen 5: Reserved",
    1 << 60: "Gen 3: Reserved, Gen 5: Hardware DC Bus Over-voltage Fault",
    1 << 61: "Reserved",
    1 << 62: "Resolver Not Connected",
    1 << 63: "Reserved",
}
