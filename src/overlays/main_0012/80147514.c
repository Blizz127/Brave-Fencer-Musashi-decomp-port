/* Owner re-derivation from retail assembly (clean input set). */
/* Eases the look vector at 0x118 toward target(+offset) by 1/8 and converts it to angles at 0x100. */
#include "psx_types.h"

typedef struct {
    s16 x;
    s16 y;
    s16 z;
    s16 pad;
} Vec80147514;

typedef struct {
    /* 0x000 */ u8 pad000[0x100];
    /* 0x100 */ s16 angle[3];
    /* 0x106 */ u8 pad106[0x12];
    /* 0x118 */ s16 cur[3];
    /* 0x11E */ u8 pad11E[2];
    /* 0x120 */ Vec80147514 target;
    /* 0x128 */ s16 offset[3];
    /* 0x12E */ s16 useOffset;
} Obj80147514;

extern s16 func_80012C6C(s16 cur, s16 target, s16 shift);
extern void func_800129CC(Vec80147514 *dir, s16 *angle);

void func_80147514(Obj80147514 *obj)
{
    Vec80147514 v;
    Vec80147514 spare;

    v = obj->target;
    if ((v.z | (v.x | v.y)) == 0) {
        v.y = -0xFFF;
    }
    if (obj->useOffset != 0) {
        v.x += obj->offset[0];
        v.y += obj->offset[1];
        v.z += obj->offset[2];
    }
    obj->cur[0] = func_80012C6C(obj->cur[0], v.x, 8);
    obj->cur[1] = func_80012C6C(obj->cur[1], v.y, 8);
    obj->cur[2] = func_80012C6C(obj->cur[2], v.z, 8);
    v.x = obj->cur[0];
    v.y = obj->cur[1];
    v.z = obj->cur[2];
    func_800129CC(&v, obj->angle);
}
