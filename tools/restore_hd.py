"""Conservative photographic sprite restoration, not generative reconstruction."""
import argparse
from pathlib import Path
from PIL import Image, ImageFilter, ImageChops

def restore(image, scale=4):
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
    return rgb

def main():
    p = argparse.ArgumentParser()
    p.add_argument("source", type=Path)
    p.add_argument("output", type=Path)
    p.add_argument("--scale", type=int, choices=[2,4], default=4)
    a = p.parse_args()
    a.output.mkdir(parents=True, exist_ok=True)
    count = 0
    for path in sorted(a.source.glob("*.png")):
        with Image.open(path) as im:
            restore(im,a.scale).save(a.output/path.name)
        count += 1
    print(f"Restored {count} textures")

if __name__ == "__main__":
    main()
