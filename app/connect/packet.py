class PacketSchemaException(Exception):
    pass

class UnknownPacketException(Exception):
    pass

class Packet:
    """ Abstract packet """
    # Always has a data variable
    data = {}

    def verify_data(self, data):
        if data.keys() != self.fields:
            raise PacketSchemaException("Packet schema violated")

    def __init__(self):
        raise NotImplementedError("Must be subclassed")

class CANPacket(Packet):
    """ Packet for containing CAN Bus data """

    fields = {'frame_id', 'timestamp', 'signals'}

    def __init__(self, data):
        self.verify_data(data)
        self.data = data

