/* func_80171E08 [80171E08,80171EC8) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct { u8 p0[0x12]; s16 f12; } S;
typedef struct { u8 p0[6]; s16 f6; u8 p8[6]; s16 fE; u8 p10[0x10]; S *f20; } O;
typedef struct { s16 x; s16 y; s16 z; } P;

/* callers read v0: none-direct (no jal caller); retail returns the probe result / 0. */
extern s32 func_8017248C(O *o, P *p);
extern void func_80146D90(O *o);
extern s32 func_8004CFEC(s32 y, s32 x);
extern s32 func_80012DBC(s32 from, s16 to, s16 num, s16 den);
extern void func_80171EC8(O *o);

s32 func_80171E08(O *o, P *p, s32 c) {
    s32 r;
    s32 ang;

    r = func_8017248C(o, p);
    if (r != 0) {
        func_80146D90(o);
        return r;
    }
    ang = (func_8004CFEC(p->x - o->f6, p->z - o->fE) + 0x800) & 0xFFF;
    if (c != 0) {
        ang = (s16)func_80012DBC(o->f20->f12, ang, c, 1);
    }
    o->f20->f12 = ang;
    func_80171EC8(o);
    return 0;
}
