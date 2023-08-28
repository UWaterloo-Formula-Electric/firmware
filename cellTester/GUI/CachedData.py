class CachedData:
    def __init__(self, serial_port):
        self.serial_port = serial_port
        self.lines = []

    def get_unread_lines(self):
        """
        Returns the latest line of data from the serial port.\n
        This is a non blocking function that will not store any previouosly unconsumed data
        Returns None if line is not available
        """
        # Data with commas is data, otherwise consider it a status message from the cell tester
        # Ensures that there is always at least one line of data
        raw_data = self.serial_port.read_all() + self.serial_port.read_until(b"\n")
        self.lines.extend(raw_data.split(b"\n"))
        if self.lines[-1] == b'':
            self.lines = self.lines[:-1]
        self.buffer = b""
        return self.consume_lines()
    
    def consume_lines(self):
        """
        Returns all the lines of data from the serial port that have not been consumed.\n
        """
        lines = self.lines
        self.lines = []
        return lines
    
    

