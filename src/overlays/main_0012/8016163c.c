#include "psx_types.h"

typedef struct { u8 pad[0x12]; s16 v12; } Sub;
typedef struct {
    u8 pad0[0x20];
    Sub *p20;
    u8 pad24[0x148];
    u16 f16C;
    u16 f16E;
} Obj;

extern void func_801599A4(Obj *o);
extern void func_80159B3C(Obj *o);
extern void func_8015BF48(Obj *o);
extern void func_8015F2F0(Obj *o);
extern void func_8014C010(Obj *o, s32 v);
extern void func_80161D20(Obj *o, s32 a);
extern s32 func_8014A6C4(Obj *o);
extern s32 func_8004CFEC(s32 a, s32 b);
extern s16 D_801152B0;
extern s16 D_801152B4;

s32 func_8016163C(Obj *o, s32 f) {
    if (f & 1) {
        func_801599A4(o);
        func_80159B3C(o);
        func_8014C010(o, 1);
        func_80161D20(o, o->f16E);
        return 1;
    }
    if (f & 0x4000) {
        func_801599A4(o);
        func_8015BF48(o);
        func_8014C010(o, 1);
        func_80161D20(o, o->f16E);
        return 2;
    }
    if (f & 0x2000) {
        func_801599A4(o);
        func_80159B3C(o);
        func_8014C010(o, 1);
        func_80161D20(o, o->f16E);
        return 4;
    }
    if (f & 0x8000) {
        if (o->f16C == 0x15) {
            if (func_8014A6C4(o) != 0) {
                func_801599A4(o);
                func_8015F2F0(o);
                o->p20->v12 = func_8004CFEC(D_801152B0, D_801152B4);
                return 8;
            }
        } else {
            return 0;
        }
    }
    return 0;
}
