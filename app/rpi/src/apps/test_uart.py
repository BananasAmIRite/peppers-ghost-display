import lib.comms.comms as comms
import time
import lib.comms.ESPSerial as ESPSerial
import lib.PeriodicThread as PeriodicThread
# from threads.weather_query import weather_query 
# from threads.spotify_query import spotify_query, SpotifyState
from dotenv import load_dotenv
import os

# Configure the serial port
PORT = "COM8"
BAUDRATE = 115200  # Must match the baud rate of the connected device

LATITUDE  = 42.3601
LONGITUDE = -71.0589


if __name__ == "__main__":
    # spot_state = SpotifyState()
    load_dotenv()
    client_id = os.getenv("SPOTIFY_CLIENT_ID")
    client_secret = os.getenv("SPOTIFY_CLIENT_SECRET")

    ser = ESPSerial.ESPSerial(PORT, BAUDRATE)

    # thread_weather = PeriodicThread.PeriodicThread(60*30, weather_query, ser, LATITUDE, LONGITUDE)
    # thread_spotify = PeriodicThread.PeriodicThread(10, spotify_query, spot_state, ser, spi, client_id, client_secret)

    ser.open()

#    thread_weather.start()
    # thread_spotify.start()

    print("Sending start message...")
    ser.write_message(comms.SET_SCREEN, bytearray([1]))
#    ser.write_message(comms.DEVICE_WORK, bytearray())

#    time.sleep(2)

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nProgram terminated gracefully.")

    # thread_weather.stop()
    # thread_weather.join()
    # thread_spotify.stop()
    # thread_spotify.join()

    # print("Main Thread stopped. ")


    out = ser.read_all()
    with open('./output.txt', 'w') as file:
        file.write(out.decode("utf-8"))
    ser.close()
