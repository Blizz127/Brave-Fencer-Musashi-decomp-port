/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"
typedef struct {
    u32 f0;
    u16 f4;
    u16 f6;
} Dst;

typedef struct {
    u16 f0;
    u16 f2;
    u32 f4;
} Src;

typedef struct {
    u8 pad0[0x28];
    u16 f28;
    u16 f2A;
} Obj;

/* Returns retail's leftover v0 (the u16 read from s->f2, zero-extended). */
s32 func_80128EA8(Obj *o, Dst *d, Src *s) {
    s32 r;
    d->f0 = (u32)s;
    d->f4 = 0;
    d->f6 = s->f4 & 0x3F;
    o->f28 = s->f0;
    r = s->f2;
    o->f2A = r;
    return r;
}
