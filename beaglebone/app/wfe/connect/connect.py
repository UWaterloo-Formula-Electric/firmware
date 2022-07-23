from cantools.database.can.signal import NamedSignalValue

import zmq
import json

from connect.packet import Packet, CANPacket, UnknownPacketException

class QueueSerializer:
    # http://zguide.zeromq.org/page:all#Pub-Sub-Message-Envelopes

    def serialize(self, topic, msg):
        """ JSON encode our topic and message into a multipart message """
        if isinstance(msg, NamedSignalValue):
            msg = msg.name()
        msg_string = json.dumps(msg)
        return [topic.encode(), msg_string.encode()]

    def deserialize(self, serialized_msg):
        """ Decode the message """
        packet_type = serialized_msg[0].decode()
        msg = json.loads(serialized_msg[1].decode())
        return packet_type, msg


class QueueDataPublisher:

    PIPE = '/tmp/CAN-fifo'
    HWM = 200

    def __init__(self):
        self.zmq_ctx = zmq.Context()
        self.fifo_socket = self.zmq_ctx.socket(zmq.PUB)
        # Set max queue size as 100 to avoid filling RAM
        self.fifo_socket.set_hwm(self.HWM)
        # logger.info("Initializing FIFO queue at {}".format(self.PIPE))
        self.fifo_socket.bind('ipc://{}'.format(self.PIPE))
        self.serializer = QueueSerializer()

    def send(self, packet):
        if not isinstance(packet, Packet):
            raise TypeError("{} is not a Packet type".format(type(packet)))

        self.fifo_socket.send_multipart(
                self.serializer.serialize(
                    # Use the class name as the topic name
                    topic=type(packet).__name__,
                    msg=packet.data
                )
        )

class QueueDataSubscriber:

    PIPE = '/tmp/CAN-fifo'
    HWM = 200

    def __init__(self):
        self.zmq_ctx = zmq.Context()
        self.fifo_socket = self.zmq_ctx.socket(zmq.SUB)
        self.fifo_socket.connect('ipc://{}'.format(self.PIPE))
        # logger.info("Initializing FIFO queue at {}".format(self.PIPE))
        # Set max queue size as 100 to avoid filling RAM
        # self.fifo_socket.set_hwm(self.HWM)
        self.serializer = QueueSerializer()

    def subscribe_to_packet_type(self, packet):
        # A packet type of empty string means all packet types
        self.fifo_socket.setsockopt_string(zmq.SUBSCRIBE, packet)

    def recv(self):
        packet_type, data = self.serializer.deserialize(
            self.fifo_socket.recv_multipart()
        )

        if packet_type == CANPacket.__name__:
            packet = CANPacket(data)
        else:
            raise UnknownPacketException("Unknown packet type {}".format(packet_type))

        return packet
