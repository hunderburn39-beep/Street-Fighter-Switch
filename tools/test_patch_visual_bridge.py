import unittest
from patch_visual_bridge import patch

SOURCE = """static void draw_raw(UINT16 *base, UINT16 color)
{
	int x;
	if (x) { x = 1; }
}
static void draw_rle(UINT16 *base, UINT16 color)
{
}
static void draw_raw_widthpix(UINT16 *base, UINT16 color)
{
	int y;
}
static void shiftreg_clear(UINT16 *base, UINT16 *zbase)
{
	int z;
}
VIDEO_UPDATE( itech32 )
{
	int y;
}
"""

class PatchTests(unittest.TestCase):
    def test_hooks_are_inserted_at_function_ends(self):
        out = patch(SOURCE)
        self.assertTrue(out.startswith('#include <string.h>\n#include "visual_bridge.h"\n'))
        self.assertEqual(out.count("sftm_emit_blit(0, color, base);"), 2)
        self.assertEqual(out.count("sftm_emit_blit(1, color, base);"), 1)
        self.assertEqual(out.count("sftm_emit_clear(base);"), 1)
        self.assertEqual(out.count("sftm_emit_display();\n\tsftm_visual_end();"), 1)
        # hook definitions come before the first blitter function; calls stay after declarations
        self.assertLess(out.index("static void sftm_emit_blit("), out.index("static void draw_raw("))
        self.assertLess(out.index("int y;"), out.index("sftm_emit_display();"))

    def test_native_drawing_is_not_replaced(self):
        out = patch(SOURCE)
        for line in SOURCE.splitlines():
            self.assertIn(line, out)

    def test_idempotent(self):
        once = patch(SOURCE)
        self.assertEqual(patch(once), once)

    def test_unsupported_source_is_rejected(self):
        with self.assertRaises(ValueError):
            patch("static void other(void)\n{\n}\n")
        with self.assertRaises(ValueError):
            patch(SOURCE.replace("VIDEO_UPDATE( itech32 )", "VIDEO_UPDATE( other )"))

if __name__ == "__main__":
    unittest.main()
