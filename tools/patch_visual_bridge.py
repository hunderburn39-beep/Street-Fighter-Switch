"""Patch the pinned MAME2003-plus ITech32 video source to publish the blitter stream.

The native rendering path is left untouched (the core still needs it for its own
timing); the remaster renderer ignores the native framebuffer and rebuilds the
screen from the events published here: blits with every blitter register, shift
register clears and the scan-out window of each frame.
"""
from pathlib import Path
import sys

INCLUDES = '#include <string.h>\n#include "visual_bridge.h"\n'

HOOK = r'''
/* ---- SFTM remaster bridge: publishes blitter events, never alters drawing ---- */
static void sftm_emit_blit(int compressed, UINT16 color, UINT16 *base)
{
	SftmVisualOp op;
	memset(&op, 0, sizeof(op));
	sftm_visual_begin();
	op.kind = SFTM_OP_BLIT;
	op.rom_address = (grom_bank | ((VIDEO_TRANSFER_ADDRHI & 0xff) << 16) | VIDEO_TRANSFER_ADDRLO) % grom_size;
	op.palette_bank = color;
	op.flags = VIDEO_TRANSFER_FLAGS;
	op.x = VIDEO_TRANSFER_X & 0xfff;
	op.y = VIDEO_TRANSFER_Y & 0xfff;
	op.width = VIDEO_TRANSFER_WIDTH;
	op.height = ADJUSTED_HEIGHT(VIDEO_TRANSFER_HEIGHT);
	op.plane = (base == videoplane[0]) ? 0 : 1;
	op.encoding = compressed;
	op.src_xstep = VIDEO_SRC_XSTEP;
	op.src_ystep = VIDEO_SRC_YSTEP;
	op.dst_xstep = VIDEO_DST_XSTEP;
	op.dst_ystep = VIDEO_DST_YSTEP;
	op.ystep_per_x = VIDEO_YSTEP_PER_X;
	op.xstep_per_y = VIDEO_XSTEP_PER_Y;
	op.clip_left = VIDEO_LEFTCLIP;
	op.clip_right = VIDEO_RIGHTCLIP;
	op.clip_top = VIDEO_TOPCLIP;
	op.clip_bottom = VIDEO_BOTTOMCLIP;
	sftm_visual_transfer(&op);
}

static void sftm_emit_clear(UINT16 *base)
{
	SftmVisualOp op;
	memset(&op, 0, sizeof(op));
	sftm_visual_begin();
	op.kind = SFTM_OP_CLEAR;
	op.flags = VIDEO_TRANSFER_FLAGS;
	op.x = VIDEO_TRANSFER_X & 0xfff;
	op.y = VIDEO_TRANSFER_Y & 0xfff;
	op.height = ADJUSTED_HEIGHT(VIDEO_TRANSFER_HEIGHT);
	op.plane = (base == videoplane[0]) ? 0 : 1;
	sftm_visual_transfer(&op);
}

static void sftm_emit_display(void)
{
	SftmVisualOp op;
	memset(&op, 0, sizeof(op));
	sftm_visual_begin();
	op.kind = SFTM_OP_DISPLAY;
	op.a0 = VIDEO_DISPLAY_XORIGIN1;
	op.a1 = VIDEO_DISPLAY_YORIGIN1;
	op.a2 = VIDEO_DISPLAY_XORIGIN2 + VIDEO_DISPLAY_XSCROLL2;
	op.a3 = VIDEO_DISPLAY_YORIGIN2 + VIDEO_DISPLAY_YSCROLL2;
	op.a4 = itech32_vram_height;
	op.a5 = itech32_planes;
	sftm_visual_transfer(&op);
}
'''


def _function_end(source, signature):
    """Index of the closing brace of the function whose header is `signature`."""
    needle = signature + "\n{"
    if source.count(needle) != 1:
        raise ValueError("Unsupported video source: " + signature)
    pos = source.index(needle) + len(needle)
    depth = 1
    while depth:
        c = source[pos]
        depth += (c == "{") - (c == "}")
        pos += 1
    return pos - 1


def _append_before_end(source, signature, statement):
    end = _function_end(source, signature)
    return source[:end] + "\n\t" + statement + "\n" + source[end:]


def patch(source):
    if "sftm_emit_blit(" in source:
        return source
    anchor = "static void draw_raw(UINT16 *base, UINT16 color)"
    if source.count(anchor) != 1:
        raise ValueError("Unsupported video source")
    # Statements go at the end of each function so C89 declaration order holds.
    for signature, compressed in [
        (anchor, 0),
        ("static void draw_rle(UINT16 *base, UINT16 color)", 1),
        ("static void draw_raw_widthpix(UINT16 *base, UINT16 color)", 0),
    ]:
        source = _append_before_end(source, signature, f"sftm_emit_blit({compressed}, color, base);")
    source = _append_before_end(source, "static void shiftreg_clear(UINT16 *base, UINT16 *zbase)", "sftm_emit_clear(base);")
    source = _append_before_end(source, "VIDEO_UPDATE( itech32 )", "sftm_emit_display();\n\tsftm_visual_end();")
    source = source.replace(anchor, HOOK + "\n" + anchor, 1)
    return INCLUDES + source


if __name__ == "__main__":
    p = Path(sys.argv[1])
    p.write_text(patch(p.read_text()))
