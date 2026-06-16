import src.lib.comms as comms
import src.lib.weather as weather
import serial
import time
import src.lib.ESPSerial as ESPSerial
import src.lib.PeriodicThread as PeriodicThread
from src.threads.weather_query import weather_query 


# Configure the serial port
PORT = "COM5"
BAUDRATE = 115200  # Must match the baud rate of the connected device




if __name__ == "__main__":

    ser = ESPSerial.ESPSerial(PORT, BAUDRATE)

    thread_weather = PeriodicThread.PeriodicThread(60*30, weather_query, ser, LATITUDE, LONGITUDE)

    ser.open()

    thread_weather.start()

    print("Sending start message...")
    ser.write_message(comms.DEVICE_START, bytearray())
    ser.write_message(comms.DEVICE_WORK, bytearray())

    time.sleep(2)

    # try:
    #     while True:
    #         time.sleep(1)
    # except KeyboardInterrupt:
    #     print("\nProgram terminated gracefully.")

    # thread_weather.stop()
    # thread_weather.join()

    # print("Main Thread stopped. ")


    out = ser.read_all()
    with open('./output.txt', 'w') as file:
        file.write(out.decode("utf-8"))
    ser.close()