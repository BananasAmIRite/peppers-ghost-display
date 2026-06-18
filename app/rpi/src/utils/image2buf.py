
def image_to_buf(img):
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

            buf.append(rgb565 & 0xFF)
            buf.append(rgb565 >> 8)
    
    return (width, height, buf)