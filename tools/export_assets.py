"""Export captured ITech32 sprites and a lossless nearest-neighbor HD baseline."""
import argparse
import hashlib
import json
from pathlib import Path
from PIL import Image
from decode_sprite import decode

def export(graphics, trace, output):
    data = graphics.read_bytes()
    output.mkdir(parents=True, exist_ok=True)
    for folder in ("original", "rgba4x"):
        (output / folder).mkdir(exist_ok=True)
    seen = {}
    errors = []
    with trace.open() as rows:
        for line_no, line in enumerate(rows, 1):
            try:
                addr,w,h,rle,alpha,bank,pal = line.strip().split("\t")
                addr,w,h,rle,alpha,bank = map(int,(addr,w,h,rle,alpha,bank))
                palette = bytes.fromhex(pal)
                if len(palette)!=768:
                    raise ValueError("Invalid palette")
                key = hashlib.sha256(line.strip().encode()).hexdigest()[:32]
                if key in seen:
                    seen[key]["uses"] += 1
                    continue
                pixels = decode(data,addr,w,h,bool(rle))
                im = Image.frombytes("P",(w,h),pixels)
                im.putpalette(palette)
                if alpha:
                    im.info["transparency"] = 255
                im.save(output/"original"/(key+".png"))
                rgba = im.convert("RGBA")
                rgba.resize((w*4,h*4),Image.Resampling.NEAREST).save(output/"rgba4x"/(key+".png"))
                seen[key] = dict(address=addr,width=w,height=h,rle=bool(rle),transparent=bool(alpha),palette_bank=bank,uses=1,scale=4)
            except (ValueError,IndexError) as exc:
                errors.append({"line":line_no,"error":str(exc)})
    (output/"manifest.json").write_text(json.dumps(seen,indent=2))
    (output/"errors.json").write_text(json.dumps(errors,indent=2))
    print(f"Exported {len(seen)} unique textures; {len(errors)} rejected records")

if __name__=="__main__":
    p=argparse.ArgumentParser()
    for name in ("graphics","trace","output"):
        p.add_argument(name,type=Path)
    a=p.parse_args()
    export(a.graphics,a.trace,a.output)
