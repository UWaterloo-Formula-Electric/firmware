from dataclasses import dataclass
from typing import Optional


@dataclass
class CellData:
    time_stamp_ms: int
    voltage_V: float

    current_A: float
    temperature_C: float
    resistance_Ohm: Optional[float] 
    def __init__(self, timestamp_ms, voltage_V, current_A, resistance_Ohm=None):
        self.time_stamp_ms = timestamp_ms
        self.voltage_V = voltage_V
        self.current_A = current_A
        self.temperature_C = 0.0
        if resistance_Ohm:
            self.resistance_Ohm = resistance_Ohm
        else:
            self.resistance_Ohm = float("inf")

    def formatted_data(self):
        if self.resistance_Ohm != float("inf"):
            return self.time_stamp_ms, self.current_A, self.voltage_V, self.resistance_Ohm
        else:
            return self.time_stamp_ms, self.current_A, self.voltage_V
