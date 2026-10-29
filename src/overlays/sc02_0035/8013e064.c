/* Owner re-derivation from retail assembly (clean input set). */
/* 3-D distance-squared via the GTE SQR opcode: loads the per-axis deltas into IR1-3, squares them
 * with SQR (sf=1), stores MAC1-3 out, sums them, and hands the total to func_800480EC (isqrt-style
 * helper) to produce the scalar distance. */
#include "psx_types.h"
#include "gte_ops.h"

typedef struct {
    u8 pad0[2];
    s16 y;    /* 0x2 */
    u8 pad4[2];
    s16 z;    /* 0x6 */
    u8 pad8[2];
    s16 w;    /* 0xA */
} Vec;

extern s32 func_800480EC(s32 sumSq);

s32 func_8013E064(Vec *a, Vec *b)
{
    s32 d[3];
    s32 sq[3];

    d[0] = a->y - b->y;
    d[1] = a->z - b->z;
    d[2] = a->w - b->w;

    gte_lwc2(9, 0, &d[0]);
    gte_lwc2(10, 4, &d[0]);
    gte_lwc2(11, 8, &d[0]);
    gte_nop();
    gte_nop();
    gte_sqr12();
    gte_swc2(25, 0, &sq[0]);
    gte_swc2(26, 4, &sq[0]);
    gte_swc2(27, 8, &sq[0]);

    return func_800480EC(sq[0] + sq[1] + sq[2]);
}
