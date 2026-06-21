
# device communications: 
# [sync 0x55 0x55], [length], [type], [payload]

# Message types: 
# 0x01: DEVICE_START
# 0x02: SCROLL_LEFT
# 0x03: SCROLL_RIGHT

"""
Mental Model:
    pi decides what screens to have available
    pi suggests what screen to currently have open
    pi can also force a screen
    esp decides what screen to actually display


    
    
"""

"""
RPI Structure
- Services - structure for API access, including spotify, camera output / gestures, weather, tasks, etc.
    - output relevant signals to later pipelines
- 

"""


# Cursor
CURSOR_SET = 0x10
CURSOR_VISIBLE = 0x11 
CURSOR_CLICK = 0x12
CURSOR_DOWN = 0x13
CURSOR_UP = 0x14

# Screen
SET_SCREEN = 0x21
SET_SCREEN_NOTIF = 0x22

# WEATHER (uart)
WEATHER = 0x30 

# TASKS (spi)
TASKS_ADD = 0x31

# what was this again? Maybe calendar events lol
# CALENDAR_ADD = 0x32

SPOTIFY_SET_SONG = 0x40
SPOTIFY_SET_IMAGE = 0x41
SPOTIFY_UPDATE_SONG = 0x42

DEBUG_SET_IMAGE = 0xF0


def send_message(serial, type, payload):
    print("sending message: ", bytearray([0x55, 0x55]) + bytearray([len(payload) + 1, type]) + payload)
    serial.write(bytearray([0x55, 0x55]) + bytearray([len(payload) + 1, type]) + payload)