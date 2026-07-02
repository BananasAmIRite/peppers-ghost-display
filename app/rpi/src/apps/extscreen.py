import time
import threading
import serial
from pynput import mouse, keyboard
import lib.comms.comms as comms
from screeninfo import get_monitors

import pystray
from PIL import Image, ImageDraw


CUBE_W = 480
CUBE_H = 320


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

        self.mouse_listener = None
        self.keyboard_listener = None

        # Start enabled by default
        self.enabled = True

        self.tray_icon = None

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
        if not self.enabled:
            return

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
        if not self.enabled:
            return

        if not self.inside:
            return

        if pressed:
            comms.send_message(self.serial, comms.CURSOR_DOWN, bytearray())
            comms.send_message(self.serial, comms.CURSOR_CLICK, bytearray())
        else:
            comms.send_message(self.serial, comms.CURSOR_UP, bytearray())

    # ----------------------------
    # Swipe (only valid while mouse is over the cube screen)
    # ----------------------------
    def swipe(self, direction: int, label: str):
        if not self.enabled:
            return

        if not self.inside:
            return

        print(f"Swipe: {label}")
        comms.send_message(
            self.serial,
            comms.PI_SWIPE,
            bytearray([direction])
        )

    # ----------------------------
    # Keyboard press (global, works regardless of window focus)
    # ----------------------------
    def on_key_press(self, key):
        if not self.enabled:
            return

        try:
            char = key.char.lower()
        except AttributeError:
            # special key (ctrl, shift, arrows, etc.) - not something we handle
            return

        if char == 'a':
            self.swipe(0x00, "Left")
        elif char == 'd':
            self.swipe(0x01, "Right")
        elif char == 'w':
            self.swipe(0x02, "Up")
        elif char == 's':
            self.swipe(0x03, "Down")

    # ----------------------------
    # Enable / Disable
    # ----------------------------
    def set_enabled(self, value: bool):
        self.enabled = value

        if not self.enabled:
            # Make sure we cleanly leave the cube state (hides cursor, etc.)
            self.exit_cube()

        print(f"Cube bridge {'enabled' if self.enabled else 'disabled'}")

        if self.tray_icon is not None:
            self.tray_icon.update_menu()
            self.tray_icon.icon = self._make_tray_image(self.enabled)

    # ----------------------------
    # Tray icon
    # ----------------------------
    def _make_tray_image(self, enabled: bool):
        size = 64
        color = (40, 200, 90) if enabled else (200, 60, 60)

        image = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        draw = ImageDraw.Draw(image)
        margin = 8
        draw.ellipse(
            [margin, margin, size - margin, size - margin],
            fill=color
        )
        return image

    def _toggle_enabled(self, icon, item):
        self.set_enabled(not self.enabled)

    def _on_exit(self, icon, item):
        self.stop()
        icon.stop()

    def _build_tray_icon(self):
        menu = pystray.Menu(
            pystray.MenuItem(
                lambda item: "Disable" if self.enabled else "Enable",
                self._toggle_enabled
            ),
            pystray.MenuItem("Exit", self._on_exit)
        )

        self.tray_icon = pystray.Icon(
            "cube_bridge",
            icon=self._make_tray_image(self.enabled),
            title="Cube Bridge",
            menu=menu
        )

    # ----------------------------
    # Start listeners
    # ----------------------------
    def start(self):
        print("Cube bridge running...")
        print("\nWhen the mouse is over the cube screen, use WASD to swipe (Ctrl+C to exit):")
        print("  W -> Swipe Up    |  A -> Swipe Left")
        print("  S -> Swipe Down  |  D -> Swipe Right\n")

        self.mouse_listener = mouse.Listener(
            on_move=self.on_move,
            on_click=self.on_click
        )
        self.mouse_listener.start()

        self.keyboard_listener = keyboard.Listener(
            on_press=self.on_key_press
        )
        self.keyboard_listener.start()

    def stop(self):
        if self.mouse_listener is not None:
            self.mouse_listener.stop()
        if self.keyboard_listener is not None:
            self.keyboard_listener.stop()

    def run(self):
        self.start()
        self._build_tray_icon()
        try:
            # tray icon owns the main thread; listeners run in background
            self.tray_icon.run()
        except KeyboardInterrupt:
            print("\nProgram terminated gracefully.")
        finally:
            self.stop()


# ----------------------------
# Entry point
# ----------------------------
if __name__ == "__main__":

    ser = serial.Serial()
    ser.port = "COM11"
    ser.baudrate = 115200

    ser.dtr = False
    ser.rts = False

    ser.open()

    bridge = CubeBridge(ser)

    try:
        bridge.run()
    finally:
        bridge.stop()

        comms.send_message(
            ser,
            comms.CURSOR_VISIBLE,
            bytearray([0x00])
        )

        ser.close()