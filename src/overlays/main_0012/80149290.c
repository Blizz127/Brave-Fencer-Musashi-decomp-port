/* Owner re-derivation from retail assembly (clean input set). */
/* Temporarily replaces the linked record's three words at +0x48/+0x4C/+0x50 with o's position, calls
 * func_80149350 and restores them (the third word is restored from the +0x4C copy, as retail does). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x48];
    s32 a; /* 0x48 */
    s32 b; /* 0x4C */
    s32 c; /* 0x50 */
} Link80149290;

typedef struct {
    u8 pad0[6];
    s16 x;    /* 0x06 */
    u8 pad8[2];
    s16 y;    /* 0x0A */
    u8 padC[2];
    s16 z;    /* 0x0E */
    u8 pad10[0x10];
    u32 link; /* 0x20: guest address of Link80149290 */
} Obj80149290;

extern void func_80149350(Obj80149290 *o);

void func_80149290(Obj80149290 *o)
{
    s32 sv[3];

    sv[0] = ((Link80149290 *)o->link)->a;
    sv[1] = ((Link80149290 *)o->link)->b;
    sv[2] = ((Link80149290 *)o->link)->b;
    ((Link80149290 *)o->link)->a = o->x;
    ((Link80149290 *)o->link)->b = o->y;
    ((Link80149290 *)o->link)->c = o->z;
    func_80149350(o);
    ((Link80149290 *)o->link)->a = sv[0];
    ((Link80149290 *)o->link)->b = sv[1];
    ((Link80149290 *)o->link)->c = sv[2];
}
