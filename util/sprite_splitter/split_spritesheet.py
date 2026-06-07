from PIL import Image

IMAGE_WIDTH = 16
IMAGE_HEIGHT = 16
TOTAL_X = 4
TOTAL_Y = 7
INPUT_FILE = "chicken.png"
OUTPUT_PREFIX = "chicken"
CONVERT_ALPHA = True

# # Replace 'input.png' with your file path
# img = Image.open(f'input/{INPUT_FILE}')

# # 2. Convert to RGBA to ensure it has an alpha channel
# img = img.convert("RGBA")

# # 3. Create a solid background image (bright magenta as transparent color)
# background = Image.new("RGBA", img.size, (255, 0, 255))

# # 4. Composite the transparent image over the background
# composite = Image.alpha_composite(background, img)


# # 5. Convert to standard RGB and save as JPG
# rgb_img = composite.convert('RGB')

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
        new_img.save(f"output/{OUTPUT_PREFIX}{idx}.bmp")
        idx += 1


rgb_img.save("output_spritesheet.bmp")