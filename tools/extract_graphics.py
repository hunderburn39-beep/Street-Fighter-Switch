"""Extract original graphics address space from a user-supplied SFTM ROM."""
import argparse
import zipfile
from pathlib import Path
import zlib
import json

BANKS = [
    (0x400000, [0x09ef29cb, 0x6f5910fa, 0xb8a2add5, 0x6b6ff867]),
    (0x400000, [0xd5d65f77, 0x90467e27, 0x903e56c2, 0xfac35686]),
    (0x020000, [0x3e1f76f7, 0x578054b6, 0x9af2f698, 0xcd38d1d6]),
]

def extract(source, output):
    output.mkdir(parents=True, exist_ok=True)
    manifest = []
    with zipfile.ZipFile(source) as archive:
        entries = {i.CRC: i for i in archive.infolist()}
        address = 0
        with (output / "graphics.bin").open("wb") as dest:
            for size, checksums in BANKS:
                bank = bytearray(size * 4)
                for lane, crc in enumerate(checksums):
                    data = archive.read(entries[crc])
                    if len(data) != size or zlib.crc32(data) != crc:
                        raise ValueError("Invalid graphics ROM")
                    bank[lane::4] = data
                    manifest.append({"file": entries[crc].filename,
                                     "address": address + lane,
                                     "stride": 4, "size": size,
                                     "crc32": f"{crc:08x}"})
                dest.write(bank)
                address += len(bank)
    (output / "graphics_manifest.json").write_text(json.dumps(manifest, indent=2))
    assert address == 0x2080000

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("rom", type=Path)
    p.add_argument("output", type=Path)
    a = p.parse_args()
    extract(a.rom, a.output)
