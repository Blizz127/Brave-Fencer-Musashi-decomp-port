/* func_80161774 [80161774,80161878) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 p0[0x12];
    u16 f12;
} S;

typedef struct {
    u8 p0[0x20];
    S *f20;
    u8 p24[0x148];
    u16 f16C;
    u16 f16E;
} O;

/* callers read v0: yes (tail callers return it); returns the action code 0/1/2/4/8. */
extern void func_8014C010(O *o, s32 a);
extern void func_80161D20(O *o, s32 a);
extern void func_801599A4(O *o);
extern void func_8015BF48(O *o);
extern s32 func_8014A6C4(O *o);
extern void func_8015F2F0(O *o);
extern s32 func_8004CFEC(s32 a, s32 b);
extern s16 D_801152B0;
extern s16 D_801152B4;

s32 func_80161774(O *o, s32 flags) {
    if (flags & 1) {
        func_8014C010(o, 1);
        func_80161D20(o, o->f16E);
        return 1;
    }
    if (flags & 0x4000) {
        func_801599A4(o);
        func_8015BF48(o);
        func_8014C010(o, 1);
        func_80161D20(o, o->f16E);
        return 2;
    }
    if (flags & 0x2000) {
        func_8014C010(o, 1);
        func_80161D20(o, o->f16E);
        return 4;
    }
    if (flags & 0x8000) {
        if (o->f16C == 0x15 && func_8014A6C4(o) != 0) {
            func_801599A4(o);
            func_8015F2F0(o);
            o->f20->f12 = func_8004CFEC(D_801152B0, D_801152B4);
            return 8;
        }
    }
    return 0;
}
