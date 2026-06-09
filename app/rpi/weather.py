import serial
import time
import struct
import requests
import json

import comms

def build_weather_payload(daily_max: float, daily_min: float, current_temp: float, weather_code: int) -> bytearray:
    payload = bytearray()
    payload += struct.pack(">f", daily_max)
    payload += struct.pack(">f", daily_min)
    payload += struct.pack(">f", current_temp)
    payload += struct.pack("B",  weather_code)
    return payload


def fetch_weather(latitude: float, longitude: float) -> dict:
    url = (
        "https://api.open-meteo.com/v1/forecast"
        f"?latitude={latitude}&longitude={longitude}"
        "&current=temperature_2m,weathercode"
        "&daily=temperature_2m_max,temperature_2m_min"
        "&temperature_unit=fahrenheit"
        "&forecast_days=1"
        "&timezone=auto"
    )
    response = requests.get(url)
    data = json.loads(response.text)

    print(data)
 
    return {
        "current_temp": data["current"]["temperature_2m"],
        "daily_max":    data["daily"]["temperature_2m_max"][0],
        "daily_min":    data["daily"]["temperature_2m_min"][0],
        "weather_code": wmo_to_weather_code(data["current"]["weathercode"]),
    }

def wmo_to_weather_code(wmo: int) -> int:
    if wmo == 0:                  return 0  # Clear sky
    if wmo <= 3:                  return 1  # Mainly/partly cloudy
    if wmo <= 67:                 return 2  # Drizzle, rain, freezing rain
    if wmo <= 77:                 return 4  # Snow
    if wmo <= 82:                 return 2  # Rain showers
    if wmo <= 86:                 return 4  # Snow showers
    if wmo <= 99:                 return 3  # Thunderstorm
    return 1

# Configure the serial port
PORT = "COM5"
BAUDRATE = 115200  # Must match the baud rate of the connected device

# Cambridge, MA — change to your location
LATITUDE  = 42.3601
LONGITUDE = -71.0589


if __name__ == "__main__":
    
    print("Fetching weather...")
    w = fetch_weather(LATITUDE, LONGITUDE)
    print(f"  Current: {w['current_temp']:.1f} F")
    print(f"  High:    {w['daily_max']:.1f} F")
    print(f"  Low:     {w['daily_min']:.1f} F")
    print(f"  Code:    {w['weather_code']}")

    payload = build_weather_payload(
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
