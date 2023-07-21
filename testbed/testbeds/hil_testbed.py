import slash
from testbeds.common_testbed import Testbed, HWManifestItem

class HILTestbed(Testbed):
    pass


class VehicleHIL(HILTestbed):
    vehicle_manifest = [
        HWManifestItem("bmu", 1),
        HWManifestItem("vcu", 2),
        HWManifestItem("pdu", 3),
        HWManifestItem("charge_cart", 4),
        HWManifestItem("motor_l", 5),
        HWManifestItem("motor_r", 6),
        HWManifestItem("dcu", 7),
        HWManifestItem("wsb_fl", 8),
        HWManifestItem("wsb_fr", 9),
        HWManifestItem("wsb_rl", 10),
        HWManifestItem("wsb_rr", 11),
        HWManifestItem("beaglebone", 12),
        HWManifestItem("imu", 13),
        HWManifestItem("charger", 14),
        HWManifestItem("debug", 15),
    ]
    hil_manifest = [
        HWManifestItem("vcu_hil", 0x2),
    ]
    


@slash.fixture
def teststand():
    return slash.g.testbed
