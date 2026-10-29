/* Owner re-derivation from retail assembly (clean input set). */
/* Consumes one glyph of a text run: when the budget D_80127548 is nonzero, steps the cursor field (u16 at +0x44, or +0x10 for the plain mode) by 1 (2 for the 0xE0+ codes) after func_80138ED0 accepts the glyph code (0x1F or c - 0x20; control codes below 0x20 are skipped), then decrements and returns the budget; returns 0 when the budget is empty or a glyph is rejected. */
#include "psx_types.h"

typedef struct {
    u8 pad0[4];
    s16 mode;
    u8 pad6[2];
    u32 flags;
    u8 padC[4];
    u16 f10;
    u8 pad12[0x32];
    u16 f44;
} Obj;

extern s32 D_80127548;
typedef struct { s32 v; } W32;
extern u8 D_800D3AB4[];
extern s32 func_80138ED0(Obj *o, s32 idx, u8 *tbl);

s32 func_80138DE0(Obj *o, u8 c, s32 st)
{
    u16 *p;

    if (D_80127548 == 0) {
        return 0;
    }
    if ((o->flags & 0x400) || o->mode == 8) {
        p = &o->f44;
    } else {
        p = &o->f10;
    }
    if (c >= 0xE0) {
        st = func_80138ED0(o, 0x1F, D_800D3AB4);
        if (st != 0) {
            return 0;
        }
        *p = *p + 2;
    } else if (c >= 0x20) {
        st = func_80138ED0(o, (u16)(c - 0x20), D_800D3AB4);
        if (st != 0) {
            return 0;
        }
        *p = *p + 1;
    }
    if (st != 0) {
        return 0;
    }
    ((W32 *)&D_80127548)->v = ((W32 *)&D_80127548)->v - 1;
    return ((W32 *)&D_80127548)->v;
}
