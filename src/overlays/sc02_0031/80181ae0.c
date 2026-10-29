/* Owner re-derivation from retail assembly (clean input set). */
/* Scans the 8-byte entries starting at a0 until one has a nonzero halfword at +6, measuring each with func_80013294 against D_80126CB4; returns the smallest 16-bit distance below limit (0xFFFF when none). */
#include "psx_types.h"

typedef struct {
    u8 pad0[6];
    s16 w6;
} Ent;

extern u8 D_80126CB4[];
extern u32 func_80013294(void *a, void *b);

u16 func_80181AE0(Ent *p, s16 limit)
{
    u32 best = 0xFFFF;
    u32 d;

    if (p->w6 == 0) {
        do {
            d = func_80013294(D_80126CB4, p);
            if ((u16)d < limit && (u32)(u16)d < (u32)(u16)best) {
                best = d;
            }
            p++;
        } while (p->w6 == 0);
    }
    return best;
}
