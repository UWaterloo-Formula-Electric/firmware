from dataclasses import dataclass
from typing import Optional

from constants import LOAD_R


@dataclass
class CellData:
    time_stamp: int
    voltage: float

    current: float
    temperature: float
    _voltage_open_circuit: Optional[float] = float("inf")
    _resistance: Optional[float] = float("inf")

    @property
    def voltage_open_circuit(self) -> float:
        return self._voltage_open_circuit

    @voltage_open_circuit.setter
    def voltage_open_circuit(self, value: float):
        self._voltage_open_circuit = value

    @property
    def resistance(self) -> float:
        self._resistance = self.calculate_internal_R()
        return self._resistance

    def calculate_internal_R(self):
        if self.current == 0:
            return 0
        else:
            return (self.voltage_open_circuit / self.current) - LOAD_R

    def formatted_data(self):
        return self.time_stamp,  self.voltage_open_circuit, self.voltage, self.current, self.temperature, self.resistance