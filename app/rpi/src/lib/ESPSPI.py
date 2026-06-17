import spidev
import struct
import time


class ESPSPI:
    MAGIC = [0xAA, 0x55]

    def __init__(self, bus: int = 0, device: int = 0,
                 max_speed_hz: int = 4000000, chunk_size: int = 4096):
        self.spi = spidev.SpiDev()
        self.spi.open(bus, device)
        self.spi.max_speed_hz = max_speed_hz
        self.spi.mode = 0
        self.chunk_size = chunk_size

    def _wait_for_ack(self, timeout_sec: float = 1.0) -> bool:
        """
        Clock out bytes one at a time until we see 0xFF (ACK) or 0x01 (NAK).
        Uses xfer2 so CS deasserts after every byte, keeping the bus clean.
        """
        start = time.time()
        while (time.time() - start) < timeout_sec:
            reply = self.spi.xfer2([0x00])
            if reply[0] == 0xFF:
                return True
            if reply[0] == 0x01:
                print("NAK received.")
                return False
            time.sleep(0.001)
        print("ACK timeout.")
        return False

    def send_packet(self, data_type: int, metadata: bytes, body: bytes) -> bool:
        metadata_len = len(metadata)
        body_len     = len(body)

        # =====================================================================
        # PHASE 0: MAGIC SYNC
        # =====================================================================
        self.spi.xfer2(self.MAGIC)
        if not self._wait_for_ack():
            print("Protocol Error: No ACK for magic sync.")
            return False

        # =====================================================================
        # PHASE 1: HEADER
        # =====================================================================
        # < = little-endian  B = uint8  H = uint16  I = uint32
        header_bytes = struct.pack("<BHI", data_type, metadata_len, body_len)
        self.spi.xfer2(list(header_bytes))
        if not self._wait_for_ack():
            print("Protocol Error: No ACK for header.")
            return False

        # =====================================================================
        # PHASE 2: METADATA
        # =====================================================================
        if metadata_len > 0:
            print(list(metadata))
            self.spi.xfer2(list(metadata))
            if not self._wait_for_ack():
                print("Protocol Error: No ACK for metadata.")
                return False

        # =====================================================================
        # PHASE 3: BODY (chunked)
        # =====================================================================
        if body_len > 0:
            bytes_sent = 0
            while bytes_sent < body_len:
                end_index = min(bytes_sent + self.chunk_size, body_len)
                chunk = body[bytes_sent:end_index]
                self.spi.xfer2(list(chunk))
                if not self._wait_for_ack():
                    print(f"Protocol Error: No ACK for body chunk at offset {bytes_sent}.")
                    return False
                bytes_sent = end_index

        return True

    def close(self):
        self.spi.close()
