import struct
import sys
import numpy as np

from PIL import Image

if len(sys.argv) != 6:
    print('Usage: palettizer.py <palette row width (integer) - if 0 then palette output file will be one row> <input palette file (.PAL)> <input image file (.PNG)> <output palette file (.PNG)> <output image file (.PNG)>')
    exit(1)

PALETTE_ROW_WIDTH = int(sys.argv[1])
INPUT_PALETTE = sys.argv[2]
INPUT_IMAGE = sys.argv[3]
OUTPUT_PALETTE = sys.argv[4]
OUTPUT_IMAGE = sys.argv[5]

PALETTE = None


# Read palette input .PAL
# Using file format documentation https://www.cyotek.com/blog/loading-microsoft-riff-palette-pal-files-with-csharp
with open(INPUT_PALETTE, 'rb') as f:
    signature = f.read(4).decode('ascii')
    print(signature)

    if signature != 'RIFF':
        print('Expected signature "RIFF", got "' + signature + '"')
        exit(1)

    data_size = struct.unpack('i', f.read(4))[0]
    print(data_size)

    form = f.read(4).decode('ascii')
    print(form)

    if form != 'PAL ':
        print('Expected form "PAL ", got "' + form + '"')
        exit(1)

    chunk_id = f.read(4).decode('ascii')
    print(chunk_id)

    if chunk_id != 'data':
        print('Expected chunk ID "data", got "' + chunk_id + '"')
        exit(1)

    chunk_size = struct.unpack('i', f.read(4))[0]
    print(chunk_size)

    pal_version = struct.unpack('<h', f.read(2))[0]
    print(pal_version)

    count = struct.unpack('<h', f.read(2))[0]
    print(count)

    if PALETTE_ROW_WIDTH == 0:
        PALETTE_ROW_WIDTH = count

    PALETTE = np.zeros((int(count / PALETTE_ROW_WIDTH), PALETTE_ROW_WIDTH, 3), 'uint8')

    for i in range(count):
        r = int.from_bytes(f.read(1), 'little')
        g = int.from_bytes(f.read(1), 'little')
        b = int.from_bytes(f.read(1), 'little')

        if PALETTE_ROW_WIDTH == count:
            PALETTE[0][i] = (r, g, b)
        else:
            PALETTE[int(i / PALETTE_ROW_WIDTH)][i % PALETTE_ROW_WIDTH] = (r, g, b)

        flags = int.from_bytes(f.read(1), 'little')
        if flags != 0:
            print('idx ' + str(i) + ' has non-empty flags' + bin(flags))


# Save palette output .PNG
out_palette_img = Image.fromarray(PALETTE, 'RGB')
out_palette_img.save(OUTPUT_PALETTE)
out_palette_width, out_palette_height = out_palette_img.size


# Process input image (match colors to palette) and save result
input_img = Image.open(INPUT_IMAGE)
input_img_width, input_img_height = input_img.size

out_img = Image.new('RGBA', input_img.size, 0)

for xi in range(input_img_width):
    for yi in range(input_img_height):
        ri, gi, bi, ai = input_img.getpixel((xi, yi))

        if ai > 0:
            match_found = False
            for xp in range(out_palette_width):
                for yp in range(out_palette_height):
                    rp, gp, bp = out_palette_img.getpixel((xp, yp))

                    if ri == rp and gi == gp and bi == bp:
                        out_img.putpixel((xi, yi), (xp + yp * PALETTE_ROW_WIDTH, 0, 0, 255))
                        match_found = True
                        break

out_img.save(OUTPUT_IMAGE)
