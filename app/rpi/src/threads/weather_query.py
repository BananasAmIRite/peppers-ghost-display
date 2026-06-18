import lib.comms.ESPSerial as ESPSerial
import src.lib.weather as weather
import lib.comms.comms as comms


def weather_query(ser: ESPSerial.ESPSerial, lat: float, long: float):
    print("Fetching weather...")
    w = weather.fetch_weather(lat, long)
    print(f"  Current: {w['current_temp']:.1f} F")
    print(f"  High:    {w['daily_max']:.1f} F")
    print(f"  Low:     {w['daily_min']:.1f} F")
    print(f"  Code:    {w['weather_code']}")
    print(f"  Night:   {w['is_night']}")

    payload = weather.build_weather_payload(w)

    ser.write_uart_message(comms.WEATHER, payload)

