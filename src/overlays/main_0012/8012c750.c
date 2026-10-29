/* func_8012C750 [8012C750,8012C820) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u16 id;
    u8 p2[0x10A];
} Ent;

typedef struct {
    u8 p0[0xA];
    u16 fA;
} O;

/* callers read v0: no (8013E4E0 ignores it), but retail sets v0 on every return path
 * (0 or func_8012C890's result), so s32.
 * D_80120194 is the sentinel one entry before the 0x60-entry pool D_801202A0. */
extern u8 D_80120194[];
extern u8 D_801202A0[];
extern s32 func_8012C890(void *o, u8 *p, u8 *q);

s32 func_8012C750(O *o) {
    Ent *f;
    Ent *s;
    Ent *t;
    s32 r;

    if ((o->fA & 0x800) != 0) {
        s = (Ent *)D_80120194;
        f = s + 96;
        for (; f != s; f--) {
            if (f->id == 0) {
                goto found;
            }
        }
        f = 0;
    } else {
        s = (Ent *)D_80120194;
        f = 0;
        if (f == 0) {
            goto scan;
        }
hit:
        f = t;
        goto found;
scan:
        f = s + 97;
        for (t = (Ent *)D_801202A0; t != f; t++) {
            if (t->id == 0) {
                goto hit;
            }
        }
        f = 0;
    }
found:
    if (f == 0) {
        r = 0;
    } else {
        o->fA |= 0x8000;
        r = func_8012C890(o, (u8 *)f, 0);
    }
    return r;
}
