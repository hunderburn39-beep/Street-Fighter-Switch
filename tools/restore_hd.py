"""Conservative photographic sprite restoration, not generative reconstruction."""
import argparse
from pathlib import Path
from PIL import Image, ImageFilter, ImageChops

def restore(image, scale=4, indexed=False):
    rgba = image.convert("RGBA")
    size = tuple(v*scale for v in rgba.size)
    # Premultiplied alpha prevents transparent RGB bleeding.
    hd = rgba.convert("RGBa").resize(size, Image.Resampling.LANCZOS).convert("RGBA")
    alpha = hd.getchannel("A")
    rgb = hd.convert("RGB")
    sharp = rgb.filter(ImageFilter.UnsharpMask(radius=1.4, percent=65, threshold=4))
    interior = alpha.filter(ImageFilter.MinFilter(5))
    rgb = Image.composite(sharp, rgb, interior)
    rgb.putalpha(alpha)
    if not indexed:
        return rgb
    # Preserve indexed 256-colour output, reserving index 255 for transparency.
    mask = alpha.point(lambda v: 255 if v >= 128 else 0)
    if mask.getextrema() == (255, 255):
        return rgb.convert("RGB").quantize(colors=256, dither=Image.Dither.NONE)
    indexed = rgb.convert("RGB").quantize(colors=255, dither=Image.Dither.NONE)
    palette = indexed.getpalette()
    indexed.putpalette(palette + [0] * (768 - len(palette)))
    indexed.paste(255, mask=ImageChops.invert(mask))
    indexed.info["transparency"] = 255
    return indexed

def main():
    p = argparse.ArgumentParser()
    p.add_argument("source", type=Path)
    p.add_argument("output", type=Path)
    p.add_argument("--scale", type=int, choices=[2,4,8], default=4)
    p.add_argument('--indexed', action='store_true', help='Legacy 256-color output')
    a = p.parse_args()
    a.output.mkdir(parents=True, exist_ok=True)
    count = 0
    for path in sorted(a.source.glob("*.png")):
        with Image.open(path) as im:
            restore(im,a.scale,a.indexed).save(a.output/path.name)
        count += 1
    print(f"Restored {count} textures")

if __name__ == "__main__":
    main()
