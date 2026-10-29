/* Owner re-derivation from retail assembly (clean input set). */
/* Builds a 3x3 fixed-point (4.12) rotation matrix from three 12-bit angles via the sin/cos table. */
#include "psx_types.h"

#define SIN(w) ((s16)(w))
#define COS(w) ((w) >> 16)

extern u32 D_800636D4; /* guest pointer to the sin/cos table: sin in the low half, cos in the high half */

void func_80020DA4(u16 *ang, s16 *m)
{
    s32 *tbl = (s32 *)D_800636D4;
    s32 x, y;
    s32 sy, sz, cz;
    s32 t0, t1, t2;

    x = tbl[ang[0] & 0xFFF];
    sz = SIN(tbl[ang[2] & 0xFFF]);
    cz = COS(tbl[ang[2] & 0xFFF]);
    sy = SIN(tbl[ang[1] & 0xFFF]);
    y = tbl[ang[1] & 0xFFF];

    t0 = (COS(x) * sz) >> 12;
    t1 = (COS(x) * cz) >> 12;
    t2 = (SIN(x) * sy) >> 12;

    m[0] = (cz * COS(y)) >> 12;
    m[1] = ((t2 * cz) >> 12) - t0;
    m[2] = ((t1 * sy) >> 12) + ((SIN(x) * sz) >> 12);
    m[3] = (sz * COS(y)) >> 12;
    m[4] = ((t2 * sz) >> 12) + t1;
    m[5] = ((t0 * sy) >> 12) - ((SIN(x) * cz) >> 12);
    m[6] = -SIN(y);
    m[7] = (COS(y) * SIN(x)) >> 12;
    m[8] = (COS(y) * COS(x)) >> 12;
}
