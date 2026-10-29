/* Owner re-derivation from retail assembly (clean input set). */
/* Clears the +0x184/+0x188/+0x18A state, drops flag 0x10, raises D_800B9A17 and D_801270C0. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x44];
    u32 flags;
    u8 pad48[0x13C];
    s32 f184;
    u16 f188;
    u16 f18A;
} Obj;

extern u8 D_800B9A17[];
extern s16 D_801270C0;
extern void func_80154A74(void *o, s32 a);

void func_8016007C(Obj *o) {
    o->flags &= ~0x10;
    o->f184 = 0;
    o->f18A = 0;
    o->f188 = 0;
    D_800B9A17[0] = 1;
    func_80154A74(o, 0x11);
    D_801270C0 = 1;
}
