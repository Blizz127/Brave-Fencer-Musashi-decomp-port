/* Owner re-derivation from retail assembly (clean input set). */
/* Scans the actor table for an active, targetable entry whose probe lands lower than the current best; stores that probe and the default marker. */
#include "psx_types.h"

typedef struct {
    s16 x;
    s16 y;
    s16 z;
    s16 w;
} V8;

extern u8 D_801202A0[];
extern V8 D_801152A8;
extern s32 func_80135D20(u32 a, u32 b, V8 *probe);

s32 func_8014FFDC(void *unused, V8 *best, V8 *out) {
    V8 cur;
    V8 probe;
    u8 *e;
    s32 found;
    found = 0;
    cur = *best;
    best->y = 0x7FFF;
    for (e = D_801202A0; e < D_801202A0 + 0x6480; e += 0x10C) {
        if (*(u16 *)e != 0 && (*(u16 *)(e + 0x5C) & 0x400) != 0 && *(u32 *)(e + 0x58) != 0) {
            probe = cur;
            if (func_80135D20(*(u32 *)(e + 0x20), *(u32 *)(e + 0x58), &probe) != 0) {
                found = 1;
                if (probe.y < best->y) {
                    *best = probe;
                    *out = D_801152A8;
                }
            }
        }
    }
    return found;
}
