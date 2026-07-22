from abc import ABC, abstractmethod
from rpi.screens import ScreenType


class HostManager(ABC):

    @abstractmethod
    def send_uart_message(self, type: int, payload: bytearray):
        pass
    
    @abstractmethod
    def send_spi_message(self, data_type: int, body: bytes):
        pass

    @abstractmethod
    def add_temp_screen(self, temp_screen: ScreenType):
        pass

    @abstractmethod
    def add_temp_notif(self, temp_screen: ScreenType):
        pass

    @abstractmethod
    def remove_temp_screen(self, temp_screen: ScreenType):
        pass