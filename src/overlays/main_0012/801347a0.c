/* Owner re-derivation from retail assembly (clean input set). */
/* Tests whether the plane-bounded move described by record p fits: clips D_8017E910 against plane 0, writes the result into D_8017E918, then checks the remaining planes. */
#include "psx_types.h"

typedef struct {
    s16 v[3];
    s16 w;
} Plane;

extern s16 *D_8017E910;
extern s16 *D_8017E914;
extern s16 *D_8017E918;
extern Plane D_801152A8;

s32 func_80134A28(s16 *a, s16 *b, s32 c);

s32 func_801347A0(s16 flag, s16 *p, Plane *base, s32 *tbl)
{
    Plane *pl = &base[p[1]];
    s32 c = tbl[p[2]];
    s32 d;
    s32 den;
    s32 q;
    s16 *src;
    s16 *dst;
    s16 *s2;
    s16 *d2;

    if (func_80134A28((s16 *)pl, D_8017E914, c) >= 0) {
        return 0;
    }
    d = func_80134A28((s16 *)pl, D_8017E910, c);
    if (d < 0) {
        return 0;
    }
    d = -d;
    den = pl->v[1] * 48;
    q = d * 48 / den;
    src = D_8017E910;
    dst = D_8017E918;
    dst[0] = src[0];
    dst[1] = src[1] + q;
    dst[2] = src[2];
    if (func_80134A28((s16 *)&base[p[3]], dst, tbl[p[4]]) < -0x2F00) {
        return 0;
    }
    if (func_80134A28((s16 *)&base[p[5]], D_8017E918, tbl[p[6]]) < -0x2F00) {
        return 0;
    }
    if (func_80134A28((s16 *)&base[p[7]], D_8017E918, tbl[p[8]]) < -0x2F00) {
        return 0;
    }
    if (flag != 0) {
        if (func_80134A28((s16 *)&base[p[9]], D_8017E918, tbl[p[10]]) < -0x2F00) {
            return 0;
        }
    }
    if ((p[0] & 0x300) != 0) {
        return 0;
    }
    s2 = D_8017E910;
    d2 = D_8017E918;
    d2[0] = s2[0];
    d2[2] = s2[2];
    D_801152A8 = *pl;
    D_8017E910[3] = ((u16 *)pl)[1];
    return 1;
}
