import unittest
from decode_sprite import decode

class SpriteTests(unittest.TestCase):
    def test_raw(self):
        self.assertEqual(decode(bytes([9,1,2,3,4]),1,2,2,False),bytes([1,2,3,4]))
    def test_literal(self):
        self.assertEqual(decode(bytes([0x84,1,2,3,255]),0,2,2,True),bytes([1,2,3,255]))
    def test_repeat_across_rows(self):
        self.assertEqual(decode(bytes([5,7,0x81,255]),0,3,2,True),bytes([7]*5+[255]))
    def test_partial_run(self):
        self.assertEqual(decode(bytes([8,42]),0,2,2,True),bytes([42]*4))
    def test_truncated(self):
        with self.assertRaises(ValueError):
            decode(bytes([0x84,1]),0,2,2,True)
    def test_transparent_index(self):
        self.assertEqual(decode(bytes([4,255]),0,2,2,True),bytes([255]*4))

if __name__ == "__main__":
    unittest.main()
