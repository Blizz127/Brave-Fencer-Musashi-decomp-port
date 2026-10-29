/* Owner re-derivation from retail assembly (clean input set). */
/* Selects a follow-up state and calls func_80131E00 when neither the +0xB4 bit nor the 0x2C probe fired. */
#include "psx_types.h"
typedef struct { u8 pad0[0xB4]; u32 fB4; u8 pad[0xC]; u32 fC4; } O;
typedef struct { u8 pad[0xC1]; u8 fC1; } P;
extern void func_80131170(O *o, s32 a, s32 b);
extern s32 func_80131CA8(O *o, s32 a);
extern s32 func_80131E00(O *o, s32 a);
/* Returns retail's leftover v0: func_80131E00's result on the early exit and when nothing was requested, else func_80131CA8's result. */
s32 func_80130278(O *o) {
    s32 r = 0;
    s32 t;
    ((P *)o)->fC1 = 0xB;
    if (o->fC4 & 1) {
        return func_80131E00(o, 0xE);
    }
    if (o->fB4 & 0x800) {
        func_80131170(o, 0, 0);
        r = 1;
    }
    t = func_80131CA8(o, 0x2C);
    if (t) r = 1;
    if (r == 0) t = func_80131E00(o, 6);
    return t;
}
