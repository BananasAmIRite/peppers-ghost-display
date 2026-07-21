import lib.comms.ESPSerial as ESPSerial
import lib.comms.ESPSPI as ESPSPI
import time
from rpi.ScreenStateManager import ScreenStateManager

# Configure the serial port
PORT = "/dev/serial0"
BAUDRATE = 115200  # Must match the baud rate of the connected device


if __name__ == "__main__":

    ser = ESPSerial.ESPSerial(PORT, BAUDRATE)
    spi = ESPSPI.ESPSPI()

    mgr = ScreenStateManager(ser, spi)


    print("Initializing...")
    mgr.init()

    print("Ready!")

    mgr.wait_program()
    
    print("Main Thread stopped. ")
              
  
    out = ser.read_all()
    with open('./output.txt', 'w') as file:
        file.write(out.decode("utf-8"))
    ser.close()
