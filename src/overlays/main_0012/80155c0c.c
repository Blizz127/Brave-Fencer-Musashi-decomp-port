/* Owner re-derivation from retail assembly (clean input set). */
/* Sets flag 2 of +0x44; when the half at +0xB8 is 0x8000 runs func_80154274 with D_800D4DD4 and finishes the object. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x44];
    u32 flags;
    u8 pad48[0x70];
    u16 fB8;
} Obj;

extern u8 D_800D4DD4[];
extern void func_80154274(Obj *o, u8 *p);
extern void func_80146CA0(Obj *o);

void func_80155C0C(Obj *o) {
    u8 unused[0x40];
    o->flags |= 2;
    if (o->fB8 == 0x8000) {
        func_80154274(o, D_800D4DD4);
        func_80146CA0(o);
    }
}
