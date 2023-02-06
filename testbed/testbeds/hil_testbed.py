import slash
from testbeds.common_testbed import Testbed


class HILTestbed(Testbed):
    pass

class VehicleHIL(HILTestbed):
    def hw_manifest(self):
        return {
            "vcu": {
                "driver": "VCU",
                "can_interface": "can1"
            },
            "vcu_sim": {
                "driver": "VCUSim",
                "can_interface": "can2"
            }
        }

@slash.fixture()
def hil_testbed():
    return slash.g.testbed 
