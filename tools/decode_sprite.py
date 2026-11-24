"""Decode ITech32 blitter source pixels using known runtime metadata."""
import argparse
from pathlib import Path

def decode(data, address, width, height, compressed):
    if not 0 <= address < len(data) or not 0 < width <= 4096 or not 0 < height <= 4096:
        raise ValueError("Invalid sprite geometry or address")
    n = width * height
    if not compressed:
        if address+n > len(data):
            raise ValueError("Truncated raw sprite")
        return bytes(data[address:address+n])
    out = bytearray()
    p = address
    while len(out) < n:
        if p >= len(data):
            raise ValueError("Truncated RLE header")
        h = data[p]
        p += 1
        count = h & 127
        take = min(count, n-len(out))
        if h & 128:
            if p+take > len(data):
                raise ValueError("Truncated literal run")
            out.extend(data[p:p+take])
            p += take
        else:
            if p >= len(data):
                raise ValueError("Truncated repeat run")
            out.extend(bytes([data[p]]) * take)
            p += 1
    return bytes(out)

def main():
    from PIL import Image
    p = argparse.ArgumentParser()
    p.add_argument("graphics", type=Path)
    p.add_argument("address", type=lambda x:int(x,0))
    p.add_argument("width", type=int)
    p.add_argument("height", type=int)
    p.add_argument("output", type=Path)
    p.add_argument("--rle", action="store_true")
    p.add_argument("--palette", type=Path, required=True, help="256 RGB entries, 768 bytes, from runtime palette bank")
    p.add_argument("--transparent", action="store_true")
    a = p.parse_args()
    palette = a.palette.read_bytes()
    if len(palette) != 768:
        raise ValueError("Expected 256 RGB palette entries")
    pixels = decode(a.graphics.read_bytes(), a.address, a.width, a.height, a.rle)
    im = Image.frombytes("P", (a.width,a.height), pixels)
    im.putpalette(palette)
    if a.transparent:
        im.info["transparency"] = 255
    im.save(a.output)

if __name__ == "__main__":
    main()
