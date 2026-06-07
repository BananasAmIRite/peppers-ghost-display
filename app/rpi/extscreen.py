import time
import serial
from pynput import mouse
import comms
from screeninfo import get_monitors
import time


CUBE_W = 320
CUBE_H = 240


class CubeBridge:
    def __init__(self, serial_port):
        self.serial = serial_port

        self.cube_monitor = self.find_cube_monitor()

        if self.cube_monitor is None:
            raise RuntimeError("Cube virtual display not found")

        self.inside = False
        self.last_x = 0
        self.last_y = 0

        self.last_mouse_update = time.time()

    # ----------------------------
    # Find virtual display
    # ----------------------------
    def find_cube_monitor(self):
        monitors = get_monitors()

        for m in monitors:
            # You can refine this detection however you want
            if m.name == '\\\\.\\DISPLAY4':
                return m

        return None

    # ----------------------------
    # Enter / exit logic
    # ----------------------------
    def enter_cube(self):
        if self.inside:
            return

        self.inside = True

        comms.send_message(
            self.serial,
            comms.CURSOR_VISIBLE,
            bytearray([0x01])
        )


    def exit_cube(self):
        if not self.inside:
            return

        self.inside = False

        comms.send_message(
            self.serial,
            comms.CURSOR_VISIBLE,
            bytearray([0x00])
        )


    # ----------------------------
    # Mouse move
    # ----------------------------
    def on_move(self, x, y):
        m = self.cube_monitor

        inside = (
            m.x <= x < m.x + m.width and
            m.y <= y < m.y + m.height
        )

        if inside and not self.inside:
            self.enter_cube()

        elif not inside and self.inside:
            self.exit_cube()

        if not self.inside:
            return

        # map to cube space
        local_x = x - m.x
        local_y = y - m.y

        cube_x = int(local_x * CUBE_W / m.width)
        cube_y = int(local_y * CUBE_H / m.height)

        self.last_x = cube_x
        self.last_y = cube_y

        if (time.time() - self.last_mouse_update) > 1 / 15:

            comms.send_message(
                self.serial,
                comms.CURSOR_SET,
                bytearray([
                    (cube_x >> 8) & 0xFF,
                    cube_x & 0xFF,
                    (cube_y >> 8) & 0xFF,
                    cube_y & 0xFF,
                ])
            )
            self.last_mouse_update = time.time()

    # ----------------------------
    # Mouse button press
    # ----------------------------
    def on_click(self, x, y, button, pressed):
        if not self.inside:
            return

        if pressed:
            comms.send_message(self.serial, comms.CURSOR_DOWN, bytearray())
            comms.send_message(self.serial, comms.CURSOR_CLICK, bytearray())
        else:
            comms.send_message(self.serial, comms.CURSOR_UP, bytearray())

    # ----------------------------
    # Start listener
    # ----------------------------
    def run(self):
        print("Cube bridge running...")

        with mouse.Listener(
            on_move=self.on_move,
            on_click=self.on_click
        ) as listener:
            listener.join()


# ----------------------------
# Entry point
# ----------------------------
if __name__ == "__main__":
    ser = serial.Serial("COM5", 115200)
    ser.dtr = False
    ser.rts = False

    bridge = CubeBridge(ser)

    try:
        bridge.run()
    finally:
        comms.send_message(
            ser,
            comms.CURSOR_VISIBLE,
            bytearray([0x00])
        )
        ser.close()