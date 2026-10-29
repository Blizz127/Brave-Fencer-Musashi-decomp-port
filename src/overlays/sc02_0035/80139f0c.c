/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[4];
    s16 f4;
    u8 pad6[2];
    s32 f8;
} Obj;

extern Obj *D_80127524;
extern s32 func_80138DB8(Obj *o, s32 a1, s32 a2);
extern void func_8002D4C8(s32 a0, s32 a1);
extern s32 func_80139B18(Obj *o);

s32 func_80139F0C(Obj *o) {
    s32 a2;
    s32 r;

    if (D_80127524 != o) {
        if ((o->f8 & 0x2000) == 0) {
            r = 4;
            o->f4 = r;
            return r;
        }
    }
    a2 = (o->f8 & 0x4000) ? 0x70 : 0x60;
    r = func_80138DB8(o, 0, a2);
    if ((s16)r != 0) {
        o->f4 = 2;
        if ((o->f8 & 0x20000) == 0) {
            o->f8 = o->f8 & ~0x20;
        }
        func_8002D4C8(0x88D, 0);
    }
    return func_80139B18(o);
}
