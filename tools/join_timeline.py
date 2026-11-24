"""Join synchronized capture streams; fail on incomplete or inconsistent data."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path

def join(blits, timeline, output):
    previous = None
    count = 0
    for raw, meta in itertools.zip_longest(blits, timeline):
        if raw is None or meta is None:
            raise ValueError("Capture streams have different record counts")
        frame, order, x, y, plane = map(int, meta.split())
        if min(frame, order, x, y) < 0 or max(x,y)>4095 or plane not in (0,1):
            raise ValueError("Invalid timeline record")
        if previous is None or frame != previous[0]:
            if order != 0 or (previous is not None and frame < previous[0]):
                raise ValueError("Invalid frame boundary")
        elif order != previous[1]+1:
            raise ValueError("Nonconsecutive draw order")
        previous = frame, order
        key = hashlib.sha256(raw.strip()).hexdigest()[:32]
        output.write(json.dumps(dict(sprite=key,frame=frame,order=order,x=x,y=y,plane=plane))+"\n")
        count += 1
    if not count:
        raise ValueError("Empty capture")
    return count

if __name__ == "__main__":
    p=argparse.ArgumentParser()
    p.add_argument("blits",type=Path)
    p.add_argument("timeline",type=Path)
    p.add_argument("output",type=Path)
    a=p.parse_args()
    temp=a.output.with_suffix(a.output.suffix+".tmp")
    try:
        with a.blits.open("rb") as b, a.timeline.open() as t, temp.open("w") as o:
            count=join(b,t,o)
        temp.replace(a.output)
        print(count)
    finally:
        temp.unlink(missing_ok=True)
