import lib.comms.comms as comms
from windows.CubeBridge import CubeBridge



# ----------------------------
# Entry point
# ----------------------------
if __name__ == "__main__":
    bridge = CubeBridge("COM11", 115200)

    try:
        bridge.run()
    finally:
        bridge.stop()

        bridge.try_send_message(
            comms.CURSOR_VISIBLE,
            bytearray([0x00])
        )

        bridge.serial.close()