import json
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
            m.write_text(json.dumps({"ryu":{"width":8,"height":8}}))
            r.write_text(json.dumps({"ryu":{"file":"ryu.png","scale":2}}))
            Image.new("RGBA",(16,16),(120,30,40,127)).save(p/"ryu.png")
            pack(m,r,p/"out",32)
            a=json.loads((p/"out/atlas.json").read_text())
            e=a["sprites"]["ryu"]
            with Image.open(p/"out/atlas-0.png") as im:
                self.assertEqual(im.getpixel((e["x"],e["y"])),(120,30,40,127))
                self.assertEqual(im.getpixel((0,0)),(0,0,0,0))
            Image.new("RGBA",(15,16)).save(p/"ryu.png")
            with self.assertRaisesRegex(ValueError,"Geometry mismatch"):
                pack(m,r,p/"bad",32)
            r.write_text(json.dumps({"unknown":{"file":"ryu.png","scale":2}}))
            with self.assertRaisesRegex(ValueError,"Unknown original"):
                pack(m,r,p/"bad",32)

if __name__=="__main__":
    unittest.main()
