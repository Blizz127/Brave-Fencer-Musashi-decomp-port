/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[6];
    s16 f6;       /* 0x6 */
    u8 pad8[0xA - 0x8];
    s16 fA;       /* 0xA */
    u8 padC[0xE - 0xC];
    s16 fE;       /* 0xE */
} Obj;

extern u16 D_800B99DA;
extern void func_8016DA04(Obj *o);
extern void func_80146A6C(s32 a0, Obj *a1, s32 a2, s32 a3, s32 a4, s32 a5, s32 a6);

void func_80153B58(Obj *o) {
    if ((D_800B99DA & 3) == 0) {
        func_8016DA04(o);
    }
    if ((D_800B99DA & 1) == 0) {
        func_80146A6C(3, o, o->f6, o->fA, o->fE, 0x8880000, 0x18009000);
    }
}
