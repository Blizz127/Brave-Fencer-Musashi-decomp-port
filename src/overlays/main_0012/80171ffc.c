/* Owner re-derivation from retail assembly (clean input set). */
/* Faces the object toward a point when func_8017248C reports 0, otherwise calls func_80146D90; returns the probe result. */
#include "psx_types.h"
typedef struct { u8 p0[0x12]; u16 f12; } S;
typedef struct { u8 p0[6]; s16 f6; u8 p8[6]; s16 fE; u8 p10[0x10]; S *f20; } O;
typedef struct { s16 x; s16 y; s16 z; } P;
extern s32 func_8017248C(O *o, P *p);
extern s32 func_8004CFEC(s32 a, s32 b);
extern void func_80172170(O *o, s32 a);
extern void func_80146D90(O *o);
s32 func_80171FFC(O *o, P *p, s32 k) {
    s32 r = func_8017248C(o, p);
    if (r != 0) {
        func_80146D90(o);
        return r;
    }
    {
        s32 a = func_8004CFEC(p->x - o->f6, p->z - o->fE);
        o->f20->f12 = (a + 0x800) & 0xFFF;
        func_80172170(o, k & 0xFF);
    }
    return 0;
}
