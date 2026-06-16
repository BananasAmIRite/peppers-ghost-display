import tkinter as tk
import src.lib.comms as comms
import serial

class App(tk.Tk):
    def __init__(self, serial):
        super().__init__()

        self.title("4:3 Window")
        self.minsize(400, 300)

        # state tracking
        self.focused = False
        self.mouse_x = 0
        self.mouse_y = 0

        # serial
        self.serial = serial

        # canvas just so you can see it
        self.canvas = tk.Canvas(self, bg="white")
        self.canvas.pack(fill="both", expand=True)

        # bind events
        self.bind("<Configure>", self.enforce_aspect_ratio)
        self.bind("<ButtonPress-1>", self.on_mouse_down)
        self.bind("<ButtonRelease-1>", self.on_mouse_up)
        self.bind("<FocusIn>", self.on_focus_in)
        self.bind("<FocusOut>", self.on_focus_out)
        self.bind("<Motion>", self.on_mouse_move)

        self._resizing = False

    # ---- 4:3 aspect enforcement ----
    def enforce_aspect_ratio(self, event):
        if self._resizing:
            return

        self._resizing = True

        w = self.winfo_width()
        h = self.winfo_height()

        target_h = int(w * 3 / 4)
        target_w = int(h * 4 / 3)

        # decide whether width or height is the driver
        if abs(target_h - h) > abs(target_w - w):
            new_w = target_w
            new_h = h
        else:
            new_w = w
            new_h = target_h

        self.geometry(f"{new_w}x{new_h}")

        self._resizing = False

    # ---- focus tracking ----
    def on_focus_in(self, event):
        self.focused = True
        
        comms.send_message(self.serial, comms.CURSOR_VISIBLE, bytearray([0x01]))

    def on_focus_out(self, event):
        self.focused = False
        comms.send_message(self.serial, comms.CURSOR_VISIBLE, bytearray([0x00]))

    def on_mouse_down(self, event):
        comms.send_message(self.serial, comms.CURSOR_DOWN, bytearray())
        comms.send_message(self.serial, comms.CURSOR_CLICK, bytearray())


        
    def on_mouse_up(self, event):
        comms.send_message(self.serial, comms.CURSOR_UP, bytearray())

    def on_mouse_move(self, event):
        self.mouse_x = event.x / self.winfo_width() * 320
        self.mouse_y = event.y / self.winfo_height() * 240

        comms.send_message(self.serial, comms.CURSOR_SET, bytearray([
            (int(self.mouse_x) >> 8) & 0xFF, 
            int(self.mouse_x) & 0xFF, 
            (int(self.mouse_y) >> 8) & 0xFF, 
            int(self.mouse_y) & 0xFF, 
        ]))





if __name__ == "__main__":
    

    ser = serial.Serial("COM5", 115200)
    ser.dtr = False 
    ser.rts = False
    app = App(ser)
    app.geometry("800x600")  # 4:3 starting point
    app.mainloop()
    app.on_focus_out(None)
    ser.close()