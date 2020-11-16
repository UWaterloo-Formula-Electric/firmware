import unittest

from car_queue import QueueSerializer

class TestSerializer(unittest.TestCase):

    def setUp(self):
        self.serializer = QueueSerializer()

        self.topic = 'test_topic'
        self.message = '{"f0": 1030300, "f1": 1.2, "f2": {"sf0": 100}}'
        self.serialized_msg = [b'test_topic', b'"{\\"f0\\": 1030300, \\"f1\\": 1.2, \\"f2\\": {\\"sf0\\": 100}}"']

    def test_serialization(self):
        try:
            self.serializer.serialize(self.topic, self.message)
        except Exception as e:
            self.fail("The following error occured during serialization: {}".format(e))

    def test_deserialization(self):
        try:
            self.serializer.deserialize(self.serialized_msg)
        except Exception as e:
            self.fail("The following error occured during deserialization: {}".format(e))

    def test_inverse(self):
        serialized = self.serializer.serialize(self.topic, self.message)
        topic, message = self.serializer.deserialize(serialized)

        self.assertEqual([self.topic, self.message], [topic, message])

class TestQueue(unittest.TestCase):
    pass

