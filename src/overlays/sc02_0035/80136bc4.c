/* Owner re-derivation from retail assembly (clean input set). */
/* Relocates a record whose +8 is 0x20: +8 points at +0x20 and the offsets at +0xC..+0x1C become pointers. */
#include "psx_types.h"

typedef struct {
    u8 pad0[8];
    u32 f8;
    u32 fC;
    u32 f10;
    u32 f14;
    u32 f18;
    u32 f1C;
} Obj;

void func_80136BC4(Obj *o) {
    if (o->f8 == 0x20) {
        o->f8 = (u32)o + 0x20;
        o->fC += (u32)o;
        o->f10 += (u32)o;
        o->f14 += (u32)o;
        o->f18 += (u32)o;
        o->f1C += (u32)o;
    }
}
