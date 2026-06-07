
# device communications: 
# [sync 0x55 0x55], [length], [type], [payload]

# Message types: 
# 0x01: DEVICE_START
# 0x02: SCROLL_LEFT
# 0x03: SCROLL_RIGHT

DEVICE_START = 0x01
CURSOR_SET = 0x10
CURSOR_VISIBLE = 0x11 
CURSOR_CLICK = 0x12
CURSOR_DOWN = 0x13
CURSOR_UP = 0x14

SCROLL_LEFT = 0x20 
SCROLL_RIGHT = 0x21 
FOCUS = 0x22

WEATHER = 0x30 
TASKS_ADD = 0x31 
CALENDAR_ADD = 0x32


def send_message(serial, type, payload):
    # print("sending message: ", bytearray([0x55, 0x55]) + bytearray([len(payload) + 1, type]) + payload)
    serial.write(bytearray([0x55, 0x55]) + bytearray([len(payload) + 1, type]) + payload)