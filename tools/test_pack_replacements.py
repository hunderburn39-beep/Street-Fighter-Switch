import json
import struct
import tempfile
import unittest
from pathlib import Path
from PIL import Image
from pack_replacements import pack

class AtlasTests(unittest.TestCase):
    def test_atlas(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)
            m=p/"manifest.json"
            r=p/"mapping.json"
            m.write_text(json.dumps({"ryu":{"address":4660,"width":8,"height":8,"rle":True,"palette_bank":768}}))
            r.write_text(json.dumps({"ryu":{"file":"ryu.png","scale":2}}))
            Image.new("RGBA",(16,16),(120,30,40,127)).save(p/"ryu.png")
            pack(m,r,p/"out",32)
            a=json.loads((p/"out/atlas.json").read_text())
            e=a["sprites"]["ryu"]
            with Image.open(p/"out/atlas-0.png") as im:
                self.assertEqual(im.getpixel((e["x"],e["y"])),(120,30,40,127))
                self.assertEqual(im.getpixel((31,31)),(0,0,0,0))
                self.assertEqual(im.getpixel((e["x"]-1,e["y"])),(120,30,40,127))  # edge bleed
            raw=(p/"out/hd-0.rgba").read_bytes()
            self.assertEqual(len(raw),32*32*4)
            self.assertEqual(raw[(e["y"]*32+e["x"])*4:(e["y"]*32+e["x"])*4+4],bytes((120,30,40,127)))
            idx=(p/"out/hd.idx").read_bytes()
            self.assertEqual(idx[:4],b"SFHD")
            self.assertEqual(struct.unpack("<5I",idx[4:24]),(1,32,1,1,2))
            self.assertEqual(struct.unpack("<11I",idx[24:68]),(4660,768,1,8,8,0,e["x"],e["y"],16,16,2))
            Image.new("RGBA",(15,16)).save(p/"ryu.png")
            with self.assertRaisesRegex(ValueError,"Geometry mismatch"):
                pack(m,r,p/"bad",32)
            r.write_text(json.dumps({"unknown":{"file":"ryu.png","scale":2}}))
            with self.assertRaisesRegex(ValueError,"Unknown original"):
                pack(m,r,p/"bad",32)

if __name__=="__main__":
    unittest.main()
