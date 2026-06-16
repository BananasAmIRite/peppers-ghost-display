from PIL import Image
import struct
from lib.ESPSPI import ESPSPI
import lib.comms as comms

spi = ESPSPI()

img = Image.open("image.png").convert("RGB")

width, height = img.size

buf = bytearray()

for y in range(height):
    for x in range(width):
        r, g, b = img.getpixel((x, y))

        rgb565 = (
            ((r & 0xF8) << 8) |
            ((g & 0xFC) << 3) |
            (b >> 3)
        )

        buf.append(rgb565 >> 8)
        buf.append(rgb565 & 0xFF)



spi.send_packet(comms.DEBUG_SET_IMAGE, struct.pack("<HH", width, height), bytes(buf))