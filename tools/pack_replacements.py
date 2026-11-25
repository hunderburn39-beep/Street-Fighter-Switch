"""Pack explicitly mapped, independently redrawn RGBA sprites into bounded atlas pages."""
import argparse
import json
from pathlib import Path
from PIL import Image

def pack(manifest, mapping, output, page_size=2048):
    original = json.loads(manifest.read_text())
    replacements = json.loads(mapping.read_text())
    output.mkdir(parents=True, exist_ok=True)
    pages = []
    entries = {}
    runtime_rows = []
    runtime_keys = set()
    x = y = row = 0
    for key, item in sorted(replacements.items()):
        if key not in original:
            raise ValueError(f"Unknown original sprite: {key}")
        source = original[key]
        scale = item["scale"]
        if scale not in (2, 4):
            raise ValueError("Unsupported scale")
        with Image.open(mapping.parent / item["file"]) as im:
            if im.mode != "RGBA":
                raise ValueError(f"RGBA required: {key}")
            sprite = im.copy()
        w, h = sprite.size
        if (w,h) != (source["width"]*scale, source["height"]*scale):
            raise ValueError(f"Geometry mismatch: {key}")
        if w+2 > page_size or h+2 > page_size:
            raise ValueError(f"Sprite exceeds atlas page: {key}")
        if x+w+2 > page_size:
            x=0
            y+=row
            row=0
        if not pages or y+h+2 > page_size:
            pages.append(Image.new("RGBA",(page_size,page_size)))
            x=y=row=0
        pages[-1].paste(sprite,(x+1,y+1))
        entries[key] = {"page":len(pages)-1,"x":x+1,"y":y+1,"width":w,"height":h,"scale":scale}
        runtime_key = (source['address'], source['palette_bank'], int(source['rle']), source['width'], source['height'])
        if runtime_key in runtime_keys:
            raise ValueError(f'Ambiguous runtime sprite identity: {key}')
        runtime_keys.add(runtime_key)
        runtime_rows.append((*runtime_key, len(pages)-1, x+1, y+1, w, h))
        x+=w+2
        row=max(row,h+2)
    for i,page in enumerate(pages):
        page.save(output/f"atlas-{i}.png")
    (output/"atlas.tsv").write_text(f"SFTM_ATLAS_V1 {len(pages)} {page_size}\n" + "".join(" ".join(map(str, row))+"\n" for row in runtime_rows))
    (output/"atlas.json").write_text(json.dumps({"version":1,"page_size":page_size,"pages":len(pages),"sprites":entries},indent=2))

if __name__ == "__main__":
    p=argparse.ArgumentParser()
    for name in ("manifest","mapping","output"):
        p.add_argument(name,type=Path)
    a=p.parse_args()
    pack(a.manifest,a.mapping,a.output)
