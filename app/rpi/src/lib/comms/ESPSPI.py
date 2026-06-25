import spidev
import struct
import time
import RPi.GPIO as GPIO
import threading


class Packet:
    def __init__(self, packet_type: int, payload: bytes):
        self.packet_type = packet_type
        self.payload = payload

    def serialize(self):
        header = struct.pack(
            "<BI",
            self.packet_type,
            len(self.payload)
        )
        return header + self.payload

class ESPSPI:
    def __init__(self, bus: int = 0, device: int = 0,
                 max_speed_hz: int = 20000000, chunk_size: int = 4096, master_status_pin = 24, slave_status_pin = 25):
        self._lock = threading.RLock()
        self.spi = spidev.SpiDev()
        self.spi.open(bus, device)
        self.spi.max_speed_hz = max_speed_hz
        self.spi.mode = 0
        self.chunk_size = chunk_size
        self.master_status_pin = master_status_pin
        self.slave_status_pin = slave_status_pin

        GPIO.setmode(GPIO.BCM)

        GPIO.setup(master_status_pin, GPIO.OUT, initial=GPIO.HIGH)
        GPIO.setup(slave_status_pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)

    def wait_for_slave_low(self):
        print("Waiting for slave low...")
        print(GPIO.input(self.slave_status_pin))
        while GPIO.input(self.slave_status_pin):
            time.sleep(0.0001)
        print("Slave low!")

    def wait_for_slave_high(self):
        print("Waiting for slave high...")
        print(GPIO.input(self.slave_status_pin))
        cnt = 0
        while not GPIO.input(self.slave_status_pin) and cnt < 1 / 0.0001:
            time.sleep(0.0001)
            cnt += 1
        print("Slave high timed out" if cnt >= 1 / 0.0001 else "Slave high!")

    def send_packet(self, data_type: int, body: bytes) -> bool:
        print("Queuing sending:", data_type)
        with self._lock:
            time.sleep(0.25)
            packet = Packet(data_type, body)
            data = packet.serialize()

            print(f"Sending {len(data)} bytes", data_type)

            GPIO.output(self.master_status_pin, GPIO.LOW)

            self.wait_for_slave_low()

            offset = 0

            while offset < len(data):
                chunk = data[offset:offset + self.chunk_size]

                # if len(chunk) < RX_SIZE:
                #     chunk += bytes(RX_SIZE - len(chunk))

                print("Transferring bytes")
#                self.spi.xfer3(chunk)

                offset += len(chunk)

                if offset >= len(data):
                    GPIO.output(self.master_status_pin, GPIO.HIGH) # output high early in case slave is too fast
                self.spi.xfer3(chunk)

                # time.sleep(0.001)

                self.wait_for_slave_high()

                if offset < len(data):
                    # time.sleep(0.001)
                    self.wait_for_slave_low()



            GPIO.output(self.master_status_pin, GPIO.HIGH)
            # print("Waiting for final slave ready...")
            self.wait_for_slave_high()


            print("Done")

    def close(self):
        with self._lock:
            self.spi.close()
