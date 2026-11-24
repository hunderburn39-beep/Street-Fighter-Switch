"""Offline neural restoration using a local Real-ESRGAN x4 model and executable."""
import argparse
import subprocess
import tempfile
from pathlib import Path
from PIL import Image

def restore(source, output, executable, models):
    with Image.open(source) as im:
        rgba = im.convert("RGBA")
    with tempfile.TemporaryDirectory() as tmp:
        inp, out = Path(tmp)/"input.png", Path(tmp)/"output.png"
        # Extend foreground colors into transparent areas before inference.
        from PIL import ImageFilter
        rgb = rgba.convert("RGB")
        alpha = rgba.getchannel("A")
        for _ in range(4):
            rgb = Image.composite(rgb, rgb.filter(ImageFilter.MaxFilter(3)), alpha)
        rgb.save(inp)
        subprocess.run([str(executable), "-i", str(inp), "-o", str(out),
                        "-m", str(models), "-n", "realesrgan-x4plus",
                        "-s", "4", "-f", "png"], check=True)
        with Image.open(out) as result:
            hd = result.convert("RGB")
        expected = (rgba.width*4, rgba.height*4)
        if hd.size != expected:
            raise ValueError("Unexpected neural output dimensions")
        hd.putalpha(alpha.resize(expected, Image.Resampling.LANCZOS))
        hd.save(output)

def main():
    p = argparse.ArgumentParser()
    p.add_argument("source", type=Path)
    p.add_argument("output", type=Path)
    p.add_argument("--executable", type=Path, required=True)
    p.add_argument("--models", type=Path, required=True)
    a = p.parse_args()
    a.output.mkdir(parents=True, exist_ok=True)
    for source in sorted(a.source.glob("*.png")):
        restore(source, a.output/source.name, a.executable.resolve(), a.models.resolve())

if __name__ == "__main__":
    main()
