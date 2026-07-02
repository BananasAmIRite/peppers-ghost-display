from enum import Enum, auto
from typing import Callable, List

from lib.comms.ESPSerial import ESPSerial  


SYNC_BYTE = 0x55
MAX_PAYLOAD = 128


class RxState(Enum):
    WAIT_SYNC1 = auto()
    WAIT_SYNC2 = auto()
    READ_LEN = auto()
    READ_TYPE = auto()
    READ_PAYLOAD = auto()


# Signature: (type: int, data: bytes, size: int) -> None
UARTHandler = Callable[[int, bytes, int], None]


class UARTComms:
    def __init__(self, name: str, serial: ESPSerial):
        self.name = name
        self.serial = serial

        self.len = 0
        self.type = 0
        self.data_index = 0
        self.buffer = bytearray(MAX_PAYLOAD)

        self.state = RxState.WAIT_SYNC1
        self.uart_handlers: List[UARTHandler] = []

    def add_uart_handler(self, handler: UARTHandler):
        self.uart_handlers.append(handler)

    def handle_packet(self, type_: int, data: bytes, size: int):
        # print(f"Packet received: {type_}")
        for handler in self.uart_handlers:
            handler(type_, data, size)

    def loop(self):
        """
        Blocking read loop — call this from whatever thread you set up.
        Reads whatever bytes are available and feeds them through the state machine.
        """
        if self.serial.in_waiting == 0:
            return

        chunk = self.serial.read(self.serial.in_waiting)
        if not chunk:
            return

        for b in chunk:
            # print(f"[{self.name}] Got value: {b}")
            self.process_byte(b)

    def process_byte(self, b: int):
        if self.state == RxState.WAIT_SYNC1:
            self.state = RxState.WAIT_SYNC2 if b == SYNC_BYTE else RxState.WAIT_SYNC1

        elif self.state == RxState.WAIT_SYNC2:
            self.state = RxState.READ_LEN if b == SYNC_BYTE else RxState.WAIT_SYNC1

        elif self.state == RxState.READ_LEN:
            self.len = b
            if self.len == 0 or self.len > MAX_PAYLOAD + 1:
                self.state = RxState.WAIT_SYNC1
            else:
                self.state = RxState.READ_TYPE

        elif self.state == RxState.READ_TYPE:
            self.type = b
            self.data_index = 0

            if self.len == 1:
                self.handle_packet(self.type, b"", 0)
                self.state = RxState.WAIT_SYNC1
            else:
                self.state = RxState.READ_PAYLOAD

        elif self.state == RxState.READ_PAYLOAD:
            self.buffer[self.data_index] = b
            self.data_index += 1

            if self.data_index >= (self.len - 1):
                self.handle_packet(self.type, bytes(self.buffer[: self.len - 1]), self.len - 1)
                self.state = RxState.WAIT_SYNC1