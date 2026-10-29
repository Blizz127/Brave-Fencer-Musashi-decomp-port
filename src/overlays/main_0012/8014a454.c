/* Owner re-derivation from retail assembly (clean input set). */
/* With flag 0x4000 of +0x3C set, returns 1 when func_80012A60(+0x42, sub angle + 0x800) is below 0x2AA. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x12];
    u16 ang;
} Sub;

typedef struct {
    u8 pad0[0x20];
    Sub *sub;
    u8 pad24[0x18];
    u16 flags;
    u8 pad3E[4];
    s16 f42;
} Obj;

extern s16 func_80012A60(s16 a, s16 b);

s32 func_8014A454(Obj *o) {
    s32 r;
    if ((o->flags & 0x4000) == 0) {
        r = 0;
    } else {
        r = func_80012A60(o->f42, (o->sub->ang + 0x800) & 0xFFF) < 0x2AA;
        if (r) {
            r = 1;
        } else {
            r = 0;
        }
    }
    return r;
}
