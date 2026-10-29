/* func_8012C658 [8012C658,8012C724) - owner re-derivation from retail assembly (clean input set). */
/* Twin of func_8012C588 with an extra s16 at +0x8: seeds a link record from entry p (or zeros), finds
 * the next free 0x10C-byte entry in the pool D_801202A0 after p (or from the pool start) and links it. */
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

/* callers read v0: yes (callers store/use it); returns func_8012C890's result or 0. */
extern s16 D_80126720; /* end of the 0x60-entry pool D_801202A0 */
extern s32 func_8012C890(void *o, u8 *p, u8 *q);

s32 func_8012C658(s16 a, s16 b, Ent *p) {
    L l;
    Ent *e;
    Ent *end;
    L *lp;
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
    l.b = b;
    l.c = 0;
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
