from slash import logger
class TestbedDriver:
    def __init__(self, name):
        self.name = name
        logger.info(f"Initializing {self.name}")

