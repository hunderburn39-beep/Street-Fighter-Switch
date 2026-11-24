"""CPU-only offline sprite restoration. Requires torch, basicsr and local x4plus weights."""
import argparse
from pathlib import Path
import numpy as np
import torch
from PIL import Image
from basicsr.archs.rrdbnet_arch import RRDBNet

def main():
    p = argparse.ArgumentParser()
    p.add_argument("source", type=Path)
    p.add_argument("output", type=Path)
    p.add_argument("--weights", type=Path, required=True)
    p.add_argument("--threads", type=int, default=2)
    a = p.parse_args()
    torch.set_num_threads(a.threads)
    model = RRDBNet(num_in_ch=3, num_out_ch=3, num_feat=64, num_block=23, num_grow_ch=32, scale=4)
    checkpoint = torch.load(a.weights, map_location="cpu", weights_only=True)
    model.load_state_dict(checkpoint.get("params_ema", checkpoint.get("params", checkpoint)))
    model.eval()
    a.output.mkdir(parents=True, exist_ok=True)
    sources = sorted(a.source.glob("*.png"))
    if not sources:
        raise SystemExit("No sprites found")
    for i, path in enumerate(sources, 1):
        with Image.open(path) as im:
            rgba = im.convert("RGBA")
        x = torch.from_numpy(np.array(rgba.convert("RGB"), dtype=np.float32)/255).permute(2,0,1).unsqueeze(0)
        with torch.inference_mode():
            y = model(x).clamp(0,1)
        rgb = Image.fromarray((y[0].permute(1,2,0).numpy()*255).round().astype(np.uint8))
        rgb.putalpha(rgba.getchannel("A").resize(rgb.size, Image.Resampling.LANCZOS))
        pending = a.output/(path.stem+".pending.png")
        rgb.save(pending)
        pending.replace(a.output/path.name)
        print(f"{i}/{len(sources)} {path.name}", flush=True)

if __name__ == "__main__":
    main()
