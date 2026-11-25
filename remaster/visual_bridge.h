#ifndef SFTM_VISUAL_BRIDGE_H
#define SFTM_VISUAL_BRIDGE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Event kinds. Zero is a blit so that zero-initialised legacy records stay valid. */
enum {
 SFTM_OP_BLIT = 0,    /* ROM sprite/strip drawn into a persistent video plane */
 SFTM_OP_CLEAR = 1,   /* shift-register copy: row y replicated over height rows */
 SFTM_OP_DISPLAY = 2  /* end-of-frame scan-out window, emitted once per frame */
};

/* Flag bits copied verbatim from the ITech32 blitter transfer-flags register. */
enum {
 SFTM_FLAG_TRANSPARENT = 0x0001, /* source pen 0xff is not written */
 SFTM_FLAG_XFLIP = 0x0002,
 SFTM_FLAG_YFLIP = 0x0004,
 SFTM_FLAG_DSTXSCALE = 0x0008,
 SFTM_FLAG_DYDXSIGN = 0x0010,
 SFTM_FLAG_DXDYSIGN = 0x0020,
 SFTM_FLAG_CLIP = 0x0400,
 SFTM_FLAG_WIDTHPIX = 0x8000
};

/* Internal native coordinates; renderer owns output resolution.
   The first nine fields are the original layout and keep their meaning.
   BLIT:    x,y = start pixel (VRAM coordinates), width,height = source size,
            plane = 0/1, encoding = 0 raw / 1 RLE, palette_bank = colour latch,
            src_xstep/src_ystep/dst_xstep/dst_ystep = 8.8 fixed point step registers, ystep_per_x/xstep_per_y = skew,
            clip_left/right/top/bottom = clip registers (valid when SFTM_FLAG_CLIP is set; [left,right) x [top,bottom)).
   CLEAR:   plane, x,y = first row, height = row count, flags = YFLIP selects direction.
   DISPLAY: a0 = plane-0 x origin, a1 = plane-0 y origin,
            a2 = plane-1 x origin + scroll, a3 = plane-1 y origin + scroll,
            a4 = VRAM height in rows, a5 = number of planes. */
typedef struct {
 uint32_t rom_address, palette_bank, flags;
 int32_t x,y,width,height,plane;
 uint32_t encoding;
 uint32_t kind;
 int32_t src_xstep, src_ystep, dst_xstep, dst_ystep, ystep_per_x, xstep_per_y;
 int32_t clip_left, clip_right, clip_top, clip_bottom;
 int32_t a0, a1, a2, a3, a4, a5;
} SftmVisualOp;

typedef struct {
 void *context;
 void (*begin_frame)(void *, uint64_t);
 void (*transfer)(void *, const SftmVisualOp *);
 void (*end_frame)(void *, uint64_t);
} SftmVisualSink;
#if defined(__GNUC__)
#define SFTM_VISUAL_API __attribute__((visibility("default")))
#else
#define SFTM_VISUAL_API
#endif
SFTM_VISUAL_API void sftm_visual_bind(const SftmVisualSink *sink);
void sftm_visual_begin(void);
void sftm_visual_transfer(const SftmVisualOp *op);
void sftm_visual_end(void);
/* Contract: core implementation must preserve persistent video planes,
   clipping, palette changes and non-blit operations. No native framebuffer
   may be presented by the remaster renderer. */
#ifdef __cplusplus
}
#endif
#endif
