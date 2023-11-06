import slash
from testbeds.common_testbed import Testbed, HWManifestItem
from hil_constants import VehicleBoardId, HilBoardId

class HILTestbed(Testbed):
    pass


class VehicleHIL(HILTestbed):
    vehicle_manifest = [
        HWManifestItem("bmu", VehicleBoardId.BMU),
        HWManifestItem("vcu", VehicleBoardId.VCU),
        HWManifestItem("pdu", VehicleBoardId.PDU),
        HWManifestItem("charge_cart", VehicleBoardId.CHARGE_CART),
        HWManifestItem("motor_l", VehicleBoardId.MOTOR_L),
        HWManifestItem("motor_r", VehicleBoardId.MOTOR_R),
        HWManifestItem("dcu", VehicleBoardId.DCU),
        HWManifestItem("wsb_fl", VehicleBoardId.WSB_FL),
        HWManifestItem("wsb_fr", VehicleBoardId.WSB_FR),
        HWManifestItem("wsb_rl", VehicleBoardId.WSB_RL),
        HWManifestItem("wsb_rr", VehicleBoardId.WSB_RR),
        HWManifestItem("beaglebone", VehicleBoardId.BEAGLEBONE),
        HWManifestItem("imu", VehicleBoardId.IMU),
        HWManifestItem("charger", VehicleBoardId.CHARGER),
        HWManifestItem("debug", VehicleBoardId.DEBUG),
    ]
    hil_manifest = [
        HWManifestItem("vcu_hil", HilBoardId.VCU_HIL),
        HWManifestItem("pdu_hil", HilBoardId.PDU_HIL),
    ]
    


@slash.fixture
def teststand():
    return slash.g.testbed
