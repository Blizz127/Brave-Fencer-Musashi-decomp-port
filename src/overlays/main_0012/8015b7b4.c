/* Owner re-derivation from retail assembly (clean input set). */
/* Starts the 0xC state when func_80149B54 accepts: runs the setup helpers, latches two fields, flags bit 1 and plays the two sound cues; returns 1 on success. */
#include "psx_types.h"
typedef struct { u8 p0[0x44]; u32 f44; u8 p48[0x6C]; u32 fB4; u8 pB8[4]; u8 fBC; u8 pBD[0x1D]; u8 fDA; u8 pDB[0xCF]; u8 f1AA; u8 p1AB[0x91]; u32 f23C; u8 p240[4]; u8 f244; } O;
extern s32 func_80149B54(O *o);
extern void func_80156648(O *o);
extern void func_80154274(O *o, u8 *p);
extern void func_80154A74(O *o, s32 a);
extern void func_80146994(s32 a, O *o, s32 b, s32 c);
extern void func_80147324(s32 a);
extern void func_80147300(s32 a);
extern u8 D_800D4F8C[];
s32 func_8015B7B4(O *o) {
    s32 r;
    if (func_80149B54(o) == 0) {
        r = 0;
    } else {
        func_80156648(o);
        o->f1AA = 0xC;
        o->f44 |= 2;
        o->f23C = o->fB4;
        o->f244 = o->fBC;
        func_80154274(o, D_800D4F8C);
        func_80154A74(o, 0x22);
        o->fDA = 0;
        func_80146994(5, o, 5, 0);
        func_80147324(0x455);
        func_80147300(0x5F3);
        r = 1;
    }
    return r;
}
