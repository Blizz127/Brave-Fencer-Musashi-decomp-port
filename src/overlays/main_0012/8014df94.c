/* Owner re-derivation from retail assembly (clean input set). */
/* Builds a segment from the point (+0x88,+0x8A-0x10,+0x8C) towards the object's position (y raised by -0x10),
 * mirrors x/z about the first point, and forwards both to func_8014E284 (+0x170 == 0) or func_8014E048;
 * returns the callee's result (the stack-switch wrapper stores v0). */
#include "psx_types.h"

typedef struct {
    u8 pad0[6];
    u16 x;      /* 0x06 */
    u8 pad8[2];
    u16 y;      /* 0x0A */
    u8 padC[2];
    u16 z;      /* 0x0E */
    u8 pad10[0x78];
    u16 ax;     /* 0x88 */
    u16 ay;     /* 0x8A */
    u16 az;     /* 0x8C */
    u8 pad8E[0xE2];
    s32 mode;   /* 0x170 */
} Obj8014DF94;

/* no owner prototype exists: both callees' v0 is passed through, so declared s32 */
extern s32 func_8014E284(Obj8014DF94 *o, s16 *a, s16 *b);
extern s32 func_8014E048(Obj8014DF94 *o, s16 *a, s16 *b);

s32 func_8014DF94(Obj8014DF94 *o)
{
    s16 a[3];
    s16 b[3];

    a[0] = o->ax;
    a[1] = o->ay - 0x10;
    a[2] = o->az;
    b[0] = o->x;
    b[1] = o->y - 0x10;
    b[2] = o->z;
    b[0] -= a[0];
    b[2] -= a[2];
    b[0] = a[0] - b[0];
    b[2] = a[2] - b[2];
    if (o->mode != 0) {
        return func_8014E048(o, a, b);
    }
    return func_8014E284(o, a, b);
}
