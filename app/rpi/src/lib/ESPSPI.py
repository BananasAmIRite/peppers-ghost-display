import spidev
import struct
import time


class _NAK(Exception):
    """Raised internally when the slave sends 0x01 — triggers a full retry."""
    pass


class ESPSPI:
    MAGIC = [0xAA, 0x55]

    def __init__(self, bus: int = 0, device: int = 0,
                 max_speed_hz: int = 4000000, chunk_size: int = 4096,
                 max_retries: int = 5):
        self.spi = spidev.SpiDev()
        self.spi.open(bus, device)
        self.spi.max_speed_hz = max_speed_hz
        self.spi.mode = 0
        self.chunk_size = chunk_size
        self.max_retries = max_retries

    def _wait_for_ack(self, timeout_sec: float = 1.0):
        """
        Clock out bytes one at a time until we see 0xFF (ACK) or 0x01 (NAK).
        Raises _NAK on 0x01 so the caller can restart the whole transmission.
        Raises TimeoutError if no response arrives in time.
        """
        start = time.time()
        while (time.time() - start) < timeout_sec:
            reply = self.spi.xfer2([0x00])
            if reply[0] == 0xFF:
                return
            if reply[0] == 0x01:
                print("NAK received — restarting transmission.")
                raise _NAK()
            time.sleep(0.001)
        raise TimeoutError("ACK timeout.")

    def _attempt(self, data_type: int, metadata: bytes, body: bytes):
        """Single attempt at sending the full packet. Raises _NAK or TimeoutError on failure."""
        metadata_len = len(metadata)
        body_len     = len(body)

        # PHASE 0: MAGIC SYNC
        self.spi.xfer2(self.MAGIC)
        self._wait_for_ack()

        # PHASE 1: HEADER  (< = little-endian  B = uint8  H = uint16  I = uint32)
        header_bytes = struct.pack("<BHI", data_type, metadata_len, body_len)
        self.spi.xfer2(list(header_bytes))
        self._wait_for_ack()

        # PHASE 2: METADATA
        if metadata_len > 0:
            self.spi.xfer2(list(metadata))
            self._wait_for_ack()

        # PHASE 3: BODY (chunked)
        if body_len > 0:
            bytes_sent = 0
            while bytes_sent < body_len:
                end_index = min(bytes_sent + self.chunk_size, body_len)
                self.spi.xfer2(list(body[bytes_sent:end_index]))
                self._wait_for_ack()
                bytes_sent = end_index

    def send_packet(self, data_type: int, metadata: bytes, body: bytes) -> bool:
        """
        Send a packet, retrying from the very beginning on NAK or timeout.
        Returns True on success, False if max_retries is exhausted.
        """
        for attempt in range(1, self.max_retries + 1):
            try:
                self._attempt(data_type, metadata, body)
                return True
            except _NAK:
                print(f"Retry {attempt}/{self.max_retries} after NAK.")
            except TimeoutError as e:
                print(f"Retry {attempt}/{self.max_retries} after timeout: {e}")

        print("send_packet failed: max retries exhausted.")
        return False

    def close(self):
        self.spi.close()