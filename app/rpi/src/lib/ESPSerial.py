import threading
import serial
import time

class ESPSerial:
    def __init__(self, port, baudrate=115200, timeout=1.0):
        """
        Initializes the thread-safe ESP Serial wrapper.
        Uses RLock so the same thread can re-acquire the lock without deadlocking.
        """
        self._lock = threading.RLock()
        self.ser = serial.Serial()
        self.ser.port = port
        self.ser.baudrate = baudrate
        self.ser.timeout = timeout
        
        # ESP32/ESP8266 specific boot pinning configuration
        self.ser.dtr = False
        self.ser.rts = False

    def open(self):
        """Safely opens the serial port."""
        with self._lock:
            if not self.ser.is_open:
                self.ser.open()
                # Brief pause to let ESP hardware stabilize after boot lines clear
                time.sleep(0.1)

    def close(self):
        """Safely closes the serial port."""
        with self._lock:
            if self.ser.is_open:
                self.ser.close()

    
    def write_message(self, type: int, payload: bytearray):
        with self._lock:
            if not self.ser.is_open:
                raise serial.SerialException("Port is closed.")
            print("sending message: ", bytearray([0x55, 0x55]) + bytearray([len(payload) + 1, type]) + payload)
            return self.ser.write(bytearray([0x55, 0x55]) + bytearray([len(payload) + 1, type]) + payload)

    def read(self, size: int = 1) -> bytes:
        """Safely reads a specific number of raw bytes from the ESP."""
        with self._lock:
            if not self.ser.is_open:
                raise serial.SerialException("Port is closed.")
            return self.ser.read(size)

    def readline(self) -> bytes:
        """Safely reads a line terminating in '\\n' from the ESP."""
        with self._lock:
            if not self.ser.is_open:
                raise serial.SerialException("Port is closed.")
            return self.ser.readline()
        
    def read_all(self) -> (bytes | None):
        with self._lock:
            if not self.ser.is_open:
                raise serial.SerialException("Port is closed.")
            return self.ser.read_all()

    def flush(self):
        """Safely flushes both input and output buffers."""
        with self._lock:
            if self.ser.is_open:
                self.ser.reset_input_buffer()
                self.ser.reset_output_buffer()