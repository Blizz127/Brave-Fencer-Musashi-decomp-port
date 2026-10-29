/* Owner re-derivation from retail assembly (clean input set). */
/* Copies two 16-word palettes from `colors` into the two CLUT upload packets and submits both; returns `colors`. */
#include "psx_types.h"

extern void func_800D2E64(u32 *, s32);
extern u32 D_800DB52C[];
extern u32 D_800DB530[];
extern u32 D_800DB570[];
extern u32 D_800DB5B0[];

u32 *func_800D2B64(u32 *colors) {
    u32 *src;
    u32 *dst;
    s32 n;

    src = colors;
    dst = D_800DB530;
    for (n = 15; n != -1; n--) {
        *dst++ = *src++;
    }
    dst = D_800DB570;
    src = colors + 16;
    for (n = 15; n != -1; n--) {
        *dst++ = *src++;
    }
    func_800D2E64(D_800DB52C, 32);
    func_800D2E64(D_800DB5B0, 32);
    return colors;
}
