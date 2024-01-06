from dataclasses import dataclass

@dataclass
class SimpleVehicleModelParameters:
    """SimpleVehicleModelParameters"""
    mass_kg: float = 260 + 68 # 260kg car + 68kg driver
    tire_u: float = 0.7
    gravity: float = 9.81
    wheel_diameter: float = 9.81

