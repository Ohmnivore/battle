This is a simple tool that was quickly hacked together to extract palette information from an asset & palette exported using the *mgba* GBA emulator. This palette info is used for **[palette shifting](https://fouramgames.com/blog/sonic-battle-renderer-palette-shifting)**.

In short for every pixel in the asset the tool finds the pixel in the palette that has the exact same color and stores its index.

Run `python convert.py` in this directory to convert the images to palette mode (color indexes are stored in the red channel) and the `.pal` palette to `.png` (palette entries arranged in a single row).

Some palettes in *Sonic Battle* contain the same color more than once which makes it impossible to pick which index was used by color. I had to hex-edit palettes directly in the `.rom` file so that every index that would be used for palette shifting was a unique color in its palette.
