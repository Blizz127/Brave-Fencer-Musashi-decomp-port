/* Owner re-derivation from retail assembly (clean input set). */
/* Probes two points below the object; stores the higher hit (or 0x7FFF floor) at 0x78 and clears/sets flag bit 0 of 0x7E. */
#include "psx_types.h"

typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
    s16 pad;
} SVec;

typedef struct {
    u8 pad0[6];
    s16 x;
    u8 pad8[2];
    s16 y;
    u8 padC[2];
    s16 z;
    u8 pad10[0x78 - 0x10];
    s16 hx;
    s16 hy;
    s16 hz;
    u16 flags;
    SVec norm;
} Obj8014FE60;

extern SVec D_801152A8;

extern s32 func_8014FFDC(Obj8014FE60 *obj, SVec *pos, SVec *norm);
extern s32 func_80150150(Obj8014FE60 *obj, SVec *pos);

s32 func_8014FE60(Obj8014FE60 *obj)
{
    SVec a;
    SVec b;
    SVec n;
    s32 hitA;
    s32 hitB;

    a.vx = b.vx = obj->x;
    a.vy = b.vy = obj->y - 16;
    a.vz = b.vz = obj->z;
    hitA = func_8014FFDC(obj, &a, &n);
    hitB = func_80150150(obj, &b);
    if (hitB == 0) {
        b.vy = 0x7FFF;
    }
    if ((hitA | hitB) != 0) {
        if (a.vy < b.vy) {
            obj->hx = a.vx;
            obj->hy = a.vy;
            obj->hz = a.vz;
            obj->norm = n;
        } else {
            obj->hx = b.vx;
            obj->hy = b.vy;
            obj->hz = b.vz;
            obj->norm = D_801152A8;
        }
        obj->flags &= ~1;
        return 1;
    }
    obj->hx = a.vx;
    obj->hy = 0x7FFF;
    obj->hz = a.vz;
    obj->flags |= 1;
    return 0;
}
