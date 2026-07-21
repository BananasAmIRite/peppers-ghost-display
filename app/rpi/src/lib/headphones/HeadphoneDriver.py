from abc import ABC, abstractmethod

import struct

class HeadphoneDriver(ABC):

    @abstractmethod
    def is_available(self):
        """Return whether the device looks reachable without mutating transport state."""
        pass

    @abstractmethod
    def get_battery(self): # 0-100
        pass

    def get_extra_info(self):
        return bytes()

    def get_data_packet(self):
        batt = self.get_battery()
        return struct.pack("<B", batt) + self.get_extra_info()