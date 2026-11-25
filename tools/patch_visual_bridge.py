"""Experimental ITech32 bridge patch; preserves native internal rendering."""
from pathlib import Path
import sys
from instrument_itech32 import patch as capture_patch

def patch(source):
    source = capture_patch(source)
    source = source.replace('#include <stdio.h>', '#include <stdio.h>\n#include "visual_bridge.h"')
    needle = '    static FILE *out, *timeline;'
    start = source.index(needle)
    end = source.index('\n}', start)
    source = source[:start] + '''    SftmVisualOp op;
    sftm_visual_begin();
    op.rom_address = (grom_bank | ((VIDEO_TRANSFER_ADDRHI & 0xff) << 16) | VIDEO_TRANSFER_ADDRLO) % grom_size;
    op.palette_bank = color;
    op.flags = VIDEO_TRANSFER_FLAGS;
    op.x = VIDEO_TRANSFER_X & 0xfff;
    op.y = VIDEO_TRANSFER_Y & 0xfff;
    op.width = VIDEO_TRANSFER_WIDTH;
    op.height = ADJUSTED_HEIGHT(VIDEO_TRANSFER_HEIGHT);
    op.plane = base == videoplane[0] ? 0 : 1;
    op.encoding = compressed;
    sftm_visual_transfer(&op);''' + source[end:]
    source = source.replace('remaster_frame++; remaster_order = 0;', 'sftm_visual_begin(); sftm_visual_end(); remaster_frame++; remaster_order = 0;')
    return source

if __name__ == '__main__':
    p = Path(sys.argv[1])
    p.write_text(patch(p.read_text()))
