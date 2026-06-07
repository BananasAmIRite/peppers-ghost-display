import serial
import time


# Configure the serial port
PORT = "COM5"
BAUDRATE = 115200  # Must match the baud rate of the connected device



ser = serial.Serial(PORT, 115200)

print("Sending message...")
send_message(ser, 0x15, b'1234')
print("Done sending message. Reading...")
time.sleep(2)
out = ser.read_all()
print(out)