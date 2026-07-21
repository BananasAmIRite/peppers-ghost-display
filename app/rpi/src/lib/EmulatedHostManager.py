from lib.HostManager import HostManager

class EmualtedHostManager(HostManager):

    def send_uart_message(self, type: int, payload: bytearray):
        print("Sending uart message", type, payload.hex(" "))
    
    def send_spi_message(self, data_type: int, body: bytes):
        print("Sending SPI message: ", data_type, body.hex(" "))