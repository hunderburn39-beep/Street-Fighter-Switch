import unittest
import io
import json
import hashlib
from join_timeline import join

class TimelineTests(unittest.TestCase):
    def run_join(self, raw, meta):
        out = io.StringIO()
        count = join(io.BytesIO(raw), io.StringIO(meta), out)
        return count, [json.loads(s) for s in out.getvalue().splitlines()]

    def test_identity_and_order(self):
        n, rows = self.run_join(b'a\nb\na\n', '0 0 12 20 0\n0 1 13 20 1\n2 0 14 20 0\n')
        self.assertEqual(n, 3)
        self.assertEqual(rows[0]['sprite'], hashlib.sha256(b'a').hexdigest()[:32])
        self.assertEqual(rows[0]['sprite'], rows[2]['sprite'])
        self.assertEqual(rows[1]['plane'], 1)
        self.assertEqual(rows[2]['frame'], 2)

    def test_invalid(self):
        for raw, meta in [(b'a\n', ''), (b'a\n', '0 1 0 0 0\n'), (b'a\n', '0 0 4096 0 0\n'), (b'a\nb\n','2 0 0 0 0\n1 0 0 0 0\n'), (b'', '')]:
            with self.subTest(meta=meta), self.assertRaises(ValueError):
                self.run_join(raw, meta)

if __name__ == '__main__':
    unittest.main()
