/* Owner re-derivation from retail assembly (clean input set). */
/* Spawn/respawn a particle every (1<<arg3) frames (tested via *arg0's low
 * bits): place it randomly within +-arg2 of arg1 on x/y/z (func_8005C49C is
 * a PRNG; its first call in a burst reuses whatever seed is already in the
 * caller's a0, matching retail's leftover-argument idiom), clear a few
 * fields, copy arg1's 4th word, and poke a flag through the particle's
 * owned sub-object. Then always advance *arg0. */
#include "psx_types.h"

typedef struct {
    u16 x, y, z, w;
} Src1;

typedef struct {
    s16 x, y, z;
} Src2;

typedef struct {
    u8 pad0[6];
    s16 f6;
    u8 pad1[2];
    s16 fA;
    u8 pad2[2];
    s16 fE;
    s32 f10;
    s32 f14;
    s32 f18;
    u8 pad3[4];
    u8 *f20;
    u8 pad4[0x10];
    u16 f34;
} Obj;

extern s32 func_8005C49C();
extern u8 *func_8012913C(u16 id);

#ifdef __mips__
#define RETURN_NO_VALUE return
#else
#define RETURN_NO_VALUE return 0
#endif

s32 func_801437D8(u16 *arg0, Src1 *arg1, Src2 *arg2, s32 arg3)
{
    Obj *o;
    s16 half;
    s32 rnd;
    s32 zval;

    if (!(*arg0 & (s16)((1 << arg3) - 1))) {
        o = (Obj *)func_8012913C(0x23);
        if (o != 0) {
            rnd = func_8005C49C();
            half = arg2->x;
            o->f6 = (s16)((arg1->x + (rnd % (s32)(half * 2))) - half);

            rnd = func_8005C49C(half);
            half = arg2->y;
            o->fA = (s16)((arg1->y + (rnd % (s32)(half * 2))) - half);

            rnd = func_8005C49C(half);
            half = arg2->z;
            rnd = rnd % (s32)(half * 2);
            zval = arg1->z;
            o->f18 = 0;
            o->f14 = 0;
            o->f10 = 0;
            o->fE = (s16)((zval + rnd) - half);

            o->f34 = (u16)arg1->w;
            *(u16 *)(o->f20 + 0x2C) = 0xC008;
        }
    }
    *arg0 += 1;
    RETURN_NO_VALUE;
}
