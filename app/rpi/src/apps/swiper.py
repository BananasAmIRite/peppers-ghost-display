import time
import msvcrt
import serial
import lib.comms.comms as comms

if __name__ == '__main__':

    ser = serial.Serial()
    ser.port = "COM11"
    ser.baudrate = 115200

    ser.dtr = False
    ser.rts = False

    ser.open()

    print("\nControl device screens using WASD keys (Ctrl+C to exit):")
    print("  W -> Swipe Up    |  A -> Swipe Left")
    print("  S -> Swipe Down  |  D -> Swipe Right\n")

    try:
        while True:
            # Non-blocking check for a key press
            if msvcrt.kbhit():
                key = msvcrt.getch().decode('utf-8', errors='ignore').lower()

                if key == 'a':
                    print("Key 'A' pressed: Swiping Left")
                    comms.send_message(
                        ser,
                        comms.PI_SWIPE,
                        bytearray([0x00])
                    )
                elif key == 'd':
                    print("Key 'D' pressed: Swiping Right")
                    comms.send_message(
                        ser,
                        comms.PI_SWIPE,
                        bytearray([0x01])
                    )
                elif key == 'w':
                    print("Key 'W' pressed: Swiping Up")
                    comms.send_message(
                        ser,
                        comms.PI_SWIPE,
                        bytearray([0x02])
                    )
                elif key == 's':
                    print("Key 'S' pressed: Swiping Down")
                    comms.send_message(
                        ser,
                        comms.PI_SWIPE,
                        bytearray([0x03])
                    )

            # Minimal sleep to prevent CPU spiking
            time.sleep(0.01)

    except KeyboardInterrupt:
        print("\nProgram terminated gracefully.")