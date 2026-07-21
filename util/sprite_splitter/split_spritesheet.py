from PIL import Image

IMAGE_WIDTH = 64
IMAGE_HEIGHT = 64
TOTAL_X = 1
TOTAL_Y = 1
INPUT_FILE = "headphones_0_1.png"
OUTPUT_PREFIX = "headphones_0_1"
CONVERT_ALPHA = True

img = Image.open(f'input/{INPUT_FILE}').convert("RGBA")

rgb_img = None

if CONVERT_ALPHA:
    pixels = img.getdata()

    new_pixels = []
    for r, g, b, a in pixels:
        if a == 0:
            new_pixels.append((255, 0, 255))  # pure magenta key
        else:
            new_pixels.append((r, g, b))       # keep original RGB

    rgb_img = Image.new("RGB", img.size)
    rgb_img.putdata(new_pixels)
else:
    rgb_img = img

idx = 0
for y in range(TOTAL_Y):
    for x in range(TOTAL_X):
        box = tuple([
            x * IMAGE_WIDTH, # left 
            y * IMAGE_HEIGHT, # upper
            (x+1) * IMAGE_WIDTH, # right
            (y+1) * IMAGE_HEIGHT # lower
        ])

        new_img = rgb_img.crop(box)
        new_img.save(f"output/{OUTPUT_PREFIX}.bmp")
        idx += 1


rgb_img.save("output_spritesheet.bmp")