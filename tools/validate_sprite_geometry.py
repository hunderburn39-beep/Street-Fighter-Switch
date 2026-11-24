"""Reject replacement sprites that deviate from original silhouettes."""
import argparse
import json
from pathlib import Path
import numpy as np
from PIL import Image

def main():
    p = argparse.ArgumentParser()
    p.add_argument("original", type=Path)
    p.add_argument("candidate", type=Path)
    p.add_argument("--threshold", type=float, default=0.95)
    p.add_argument("--report", type=Path, default=Path("geometry-report.json"))
    a = p.parse_args()
    results = []
    for f in sorted(a.candidate.glob("*.png")):
        src = a.original / f.name
        if not src.exists():
            results.append(dict(file=f.name, accepted=False, reason="missing original"))
            continue
        with Image.open(src) as im:
            alpha = im.convert("RGBA").getchannel("A")
        with Image.open(f) as im:
            target = im.convert("RGBA").getchannel("A")
        dimensions_ok = target.size == (alpha.width*4, alpha.height*4)
        x = np.asarray(alpha.resize(target.size, Image.Resampling.LANCZOS)) >= 128
        y = np.asarray(target) >= 128
        union = np.count_nonzero(x | y)
        score = float(np.count_nonzero(x & y)/union) if union else 1.0
        results.append(dict(file=f.name, iou=score, dimensions_ok=dimensions_ok, accepted=dimensions_ok and score>=a.threshold))
    a.report.write_text(json.dumps(results, indent=2))
    if not results or not all(r["accepted"] for r in results):
        raise SystemExit(1)

if __name__ == "__main__":
    main()
