/* Owner re-derivation from retail assembly (clean input set). */
/* Returns 0 when the id at D_800B9A08 is in the zero-terminated list D_80186088, when flag 0x400 at +0x44 is set or the halfword at +0x1C8 is nonzero; otherwise, if bit 8 of the halfword at +0xAC is set, toggles bit 7 of the byte at D_80078E78+0x48 and returns 1. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x44];
    u32 w44;
    u8 pad48[0x64];
    u16 hAC;
    u8 padAE[0x11A];
    s16 h1C8;
} Obj;

extern s32 D_80186088[];
extern s16 D_800B9A08;
extern u8 D_80078E78[];

s32 func_80149FB0(Obj *o)
{
    s32 *p;
    s32 id;
    s16 cur;
    u8 *b;

    p = D_80186088;
    b = D_80078E78;
    id = *p++;
    if (id != 0) {
        cur = D_800B9A08;
        do {
            if (cur == id) {
                return 0;
            }
            id = *p++;
        } while (id != 0);
    }
    if ((o->w44 & 0x400) != 0) {
        return 0;
    }
    if (o->h1C8 != 0) {
        return 0;
    }
    if ((o->hAC & 0x100) != 0) {
        b[0x48] ^= 0x80;
        return 1;
    }
    return 0;
}
