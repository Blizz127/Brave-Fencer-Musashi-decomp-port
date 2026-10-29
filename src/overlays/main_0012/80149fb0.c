/* Owner re-derivation from retail assembly (clean input set). */
/* Query on an object: returns 0 if D_800B9A08 is listed in the zero-terminated id list D_8017FF38,
 * if flag 0x400 of the object's word at 0x44 is set, if its s16 at 0x1C8 is nonzero or if bit 8 of
 * its u16 at 0xAC is clear. Otherwise toggles bit 7 of the game-state byte at D_80078E78 + 0x48 and
 * returns 1. */
#include "psx_types.h"

/* Object header fields used here. */
typedef struct {
    u8 pad00[0x44];
    u32 flags;
    u8 pad48[0xAC - 0x48];
    u16 fAC;
    u8 padAE[0x1C8 - 0xAE];
    s16 f1C8;
} Obj;

extern s32 D_8017FF38[];
extern s16 D_800B9A08;
extern u8 D_80078E78[];

s32 func_80149FB0(Obj *o)
{
    s32 *p = D_8017FF38;
    u8 *g = D_80078E78;
    s32 e;

    while ((e = *p++) != 0) {
        if (D_800B9A08 == e) {
            return 0;
        }
    }
    if (o->flags & 0x400) {
        return 0;
    }
    if (o->f1C8 != 0) {
        return 0;
    }
    if (o->fAC & 0x100) {
        g[0x48] ^= 0x80;
        return 1;
    }
    return 0;
}
