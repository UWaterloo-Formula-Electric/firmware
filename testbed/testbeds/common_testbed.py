from drivers.boards.vcu import VCU
from drivers.sim_boards.vcu_sim import VCUSim
from slash import logger
from copy import copy
class Testbed:
    def __init__(self):
        self.drivers = {}
        for hw_name, hw_data in self.hw_manifest().items():
            if "driver" in hw_data and hw_data["driver"] in globals():
                params = copy(hw_data)
                params["name"] = hw_name
                del params["driver"]
                self.drivers[hw_name] = globals()[hw_data["driver"]](**params)
            else:
                logger.error("Every HW Manifest item must have an associated driver")
