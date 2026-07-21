from abc import ABC, abstractmethod

class HostManager(ABC):

    @abstractmethod
    def send_uart_message(self, type: int, payload: bytearray):
        pass
    
    @abstractmethod
    def send_spi_message(self, data_type: int, body: bytes):
        pass