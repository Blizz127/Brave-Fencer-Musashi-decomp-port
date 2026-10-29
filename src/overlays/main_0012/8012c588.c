/* func_8012C588 [8012C588,8012C658) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u16 id;
    u8 p2[4];
    u16 f6;
    u8 p8[2];
    u16 fA;
    u8 pC[2];
    u16 fE;
    u8 p10[0xFC];
} Ent;

typedef struct {
    u16 x, y, z;
    s16 a, b, c, d, e;
    s32 f;
} L;

/* callers read v0: retail sets v0 on every return path (0 or func_8012C890's result), so s32.
 * D_80126720 is the end of the 0x60-entry pool D_801202A0 (owner type s16, used by address). */
extern s16 D_80126720;
extern s32 func_8012C890(void *o, u8 *p, u8 *q);

s32 func_8012C588(s16 a, Ent *p) {
    L l;
    L *lp;
    Ent *e;
    Ent *end;
    s32 r;

    if (p != 0 && p->id != 0) {
        l.x = p->f6;
        l.y = p->fA;
        l.z = p->fE;
    } else {
        l.z = 0;
        l.y = 0;
        l.x = 0;
    }
    l.a = a;
    lp = &l;
    end = (Ent *)&D_80126720;
    l.c = 0;
    l.b = 0;
    l.f = 0;
    l.e = 0;
    l.d = 0x7FFF;
    if (p == 0) {
        e = end - 96;
    } else {
        e = p + 1;
    }
    for (; e != end; e++) {
        if (e->id == 0) {
            goto found;
        }
    }
    e = 0;
found:
    if (e == 0) {
        r = 0;
    } else {
        r = func_8012C890(lp, (u8 *)e, (u8 *)p);
    }
    return r;
}
