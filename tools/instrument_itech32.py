"""Instrument MAME2003-plus ITech32 video source for asset extraction."""
import argparse
from pathlib import Path

HOOK = r'''
static unsigned long remaster_frame, remaster_order;
static void remaster_capture(int compressed, UINT16 color, UINT16 *base)
{
    static FILE *out, *timeline;
    unsigned i;
    if (!out) out = fopen("sftm-blits.tsv", "w");
    if (!out) return;
    if (!timeline) timeline = fopen("sftm-timeline.tsv", "w");
    if (timeline) fprintf(timeline, "%lu\t%lu\t%u\t%u\t%d\n", remaster_frame, remaster_order++, (unsigned)(VIDEO_TRANSFER_X & 0xfff), (unsigned)(VIDEO_TRANSFER_Y & 0xfff), base == videoplane[0] ? 0 : 1);
    fprintf(out, "%u\t%u\t%u\t%u\t%u\t%u\t",
        (unsigned)((grom_bank | ((VIDEO_TRANSFER_ADDRHI & 0xff) << 16) | VIDEO_TRANSFER_ADDRLO) % grom_size),
        (unsigned)VIDEO_TRANSFER_WIDTH,
        (unsigned)ADJUSTED_HEIGHT(VIDEO_TRANSFER_HEIGHT),
        (unsigned)compressed,
        (unsigned)(VIDEO_TRANSFER_FLAGS & XFERFLAG_TRANSPARENT),
        (unsigned)color);
    for (i = 0; i < 256; i++)
        fprintf(out, "%06x", (unsigned)(paletteram32[color | i] & 0xffffff));
    fputc('\n', out);
    fflush(out);
}
'''
def patch(source):
    if "static void remaster_capture(" in source:
        return source
    anchor = "static void draw_raw(UINT16 *base, UINT16 color)"
    if source.count(anchor) != 1:
        raise ValueError("Unsupported video source")
    source = source.replace(anchor, HOOK + "\n" + anchor)
    for signature, compressed in [
        (anchor, 0),
        ("static void draw_rle(UINT16 *base, UINT16 color)", 1),
        ("static void draw_raw_widthpix(UINT16 *base, UINT16 color)", 0),
    ]:
        needle = signature + "\n{"
        # Insert before closing brace to preserve C89 declaration order.
        start = source.index(needle) + len(needle)
        depth = 1
        end = start
        while depth:
            c = source[end]
            depth += (c == "{") - (c == "}")
            end += 1
        source = source[:end-1] + f"\n\tremaster_capture({compressed}, color, base);\n" + source[end-1:]
    frame_anchor = "VIDEO_UPDATE( itech32 )\n{"
    if source.count(frame_anchor) != 1:
        raise ValueError("Unsupported frame callback")
    # Insert at end of callback to preserve C89 declarations.
    start = source.index(frame_anchor) + len(frame_anchor)
    depth, end = 1, start
    while depth:
        ch = source[end]
        depth += (ch == "{") - (ch == "}")
        end += 1
    source = source[:end-1] + "\n\tremaster_frame++; remaster_order = 0;\n" + source[end-1:]
    return "#include <stdio.h>\n" + source

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("source", type=Path)
    a = p.parse_args()
    a.source.write_text(patch(a.source.read_text()))
