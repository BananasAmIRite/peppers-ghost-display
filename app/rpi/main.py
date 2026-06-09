import comms
import weather
import serial
import time


# Configure the serial port
PORT = "COM5"
BAUDRATE = 115200  # Must match the baud rate of the connected device


# Cambridge, MA — change to your location
LATITUDE  = 42.3601
LONGITUDE = -71.0589

if __name__ == "__main__":
    
    print("Fetching weather...")
    w = weather.fetch_weather(LATITUDE, LONGITUDE)
    print(f"  Current: {w['current_temp']:.1f} F")
    print(f"  High:    {w['daily_max']:.1f} F")
    print(f"  Low:     {w['daily_min']:.1f} F")
    print(f"  Code:    {w['weather_code']}")

    payload = weather.build_weather_payload(
        daily_max=    w["daily_max"],
        daily_min=    w["daily_min"],
        current_temp= w["current_temp"],
        weather_code= w["weather_code"],
    )

    ser = serial.Serial()
    ser.port = PORT
    ser.baudrate = 115200

    ser.dtr = False
    ser.rts = False

    ser.open()

    print("Sending message...")
    comms.send_message(ser, comms.DEVICE_START, bytearray())

    comms.send_message(ser, comms.WEATHER, payload)

    comms.send_message(ser, comms.DEVICE_WORK, bytearray())


    out = ser.read_all()
    with open('./output.txt', 'w') as file:
        file.write(out.decode("utf-8"))
    ser.close()