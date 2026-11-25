#ifndef SFTM_VISUAL_BRIDGE_H
#define SFTM_VISUAL_BRIDGE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Internal native coordinates; renderer owns output resolution. */
typedef struct {
 uint32_t rom_address, palette_bank, flags;
 int32_t x,y,width,height,plane;
 uint32_t encoding;
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
