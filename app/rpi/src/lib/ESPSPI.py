import spidev
import struct
import time

class ESPSPI:
    def __init__(self, bus: int = 0, device: int = 0, max_speed_hz: int = 4000000, chunk_size: int = 4096):
        """
        Initializes the SPI Master Transport layer.
        
        :param bus: SPI bus number (usually 0).
        :param device: SPI chip select / device ID (usually 0 for CE0).
        :param max_speed_hz: SPI Clock speed (match with ESP32-S3 capabilities, e.g., 4MHz).
        :param chunk_size: Max size of data to transmit per chunk during the body phase.
        """
        self.spi = spidev.SpiDev()
        self.spi.open(bus, device)
        self.spi.max_speed_hz = max_speed_hz
        self.spi.mode = 0  # SPI Mode 0 (CPOL=0, CPHA=0) to match ESP32 configuration
        self.chunk_size = chunk_size

    def _wait_for_ack(self, timeout_sec: float = 1.0) -> bool:
        """
        Executes a 1-byte read transaction to capture the 0xFF ACK from the ESP32.
        """
        start_time = time.time()
        while (time.time() - start_time) < timeout_sec:
            # Send a dummy byte (0x00) to clock out the 1-byte response from the slave
            reply = self.spi.xfer2([0x00])
            if reply[0] == 0xFF:
                return True
            elif reply[0] == 0x01:
                return False
            time.sleep(0.001)  # Tiny yield to prevent pinning the CPU line
        return False

    def send_packet(self, data_type: int, metadata: bytes, body: bytes) -> bool:
        """
        Transmits a segmented packet across 3 distinct phases followed by ACKs.
        
        :param data_type: uint8_t identifier.
        :param metadata: bytes object representing arbitrary metadata.
        :param body: bytes object representing arbitrary length body.
        :return: True if sent successfully with all ACKs received, False otherwise.
        """
        metadata_len = len(metadata)
        body_len = len(body)

        # =====================================================================
        # PHASE 1: HEADER TRANSACTION
        # =====================================================================
        # Pack into struct matching C++ layout:
        # < = Little Endian
        # B = uint8_t (type)
        # H = uint16_t (metadata_len)
        # I = uint32_t (body_len)
        header_bytes = struct.pack("<BHI", data_type, metadata_len, body_len)
        print(list(header_bytes));
        # Convert packed bytes into a standard mutable integer list for spidev
        self.spi.xfer3(list(header_bytes))
        
        if not self._wait_for_ack():
            print("Protocol Error: Timeout waiting for HEADER ACK.")
            return False

        # =====================================================================
        # PHASE 2: METADATA TRANSACTION
        # =====================================================================
        if metadata_len > 0:
            # Metadata is typically short enough to transfer in one transaction
            print(list(metadata))
            self.spi.xfer3(list(metadata))
            if not self._wait_for_ack():
                print("Protocol Error: Timeout waiting for METADATA ACK.")
                return False

        # =====================================================================
        # PHASE 3: BODY TRANSACTION (CHUNKED)
        # =====================================================================
        if body_len > 0:
            bytes_sent = 0
            while bytes_sent < body_len:
                # Carve out a chunk boundary
                end_index = min(bytes_sent + self.chunk_size, body_len)
                chunk = body[bytes_sent:end_index]
                print(bytes_sent, end_index)
                # Transmit the current block
                self.spi.xfer3(list(chunk))
                
                # The ESP32 code issues an ACK after *each* successfully grabbed chunk
                if not self._wait_for_ack():
                    print(f"Protocol Error: Timeout waiting for BODY CHUNK ACK at offset {bytes_sent}.")
                    return False
                
                bytes_sent = end_index

        return True

    def close(self):
        self.spi.close()
