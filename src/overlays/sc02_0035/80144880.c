/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct Obj Obj;
typedef struct Sub Sub;
typedef struct Info Info;

struct Info {
    u8 pad0[0x6];
    u16 f6;       /* 0x6 */
    u8 pad8[0xA - 0x8];
    u16 fA;       /* 0xA */
    u8 padC[0xE - 0xC];
    u16 fE;       /* 0xE */
    u8 pad10[0x36 - 0x10];
    u16 f36;      /* 0x36 */
};

struct Sub {
    u8 pad0[0x4];
    s32 f4;       /* 0x4 */
    u16 f8;       /* 0x8 */
    u16 fA;       /* 0xA */
    u16 fC;       /* 0xC */
    u8 padE[0x10 - 0xE];
    u16 f10;      /* 0x10 */
    u8 pad12[0x18 - 0x12];
    u16 f18;      /* 0x18 */
    u16 f1A;      /* 0x1A */
    u8 pad1C[0x27 - 0x1C];
    u8 f27;       /* 0x27 */
};

struct Obj {
    u8 pad0[0x2];
    u16 f2;       /* 0x2 */
    u8 pad4[0x64 - 0x4];
    Info *f64;    /* 0x64 */
    u8 pad68[0x70 - 0x68];
    s16 f70;      /* 0x70 */
    u8 pad72[0xCC - 0x72];
    Sub *fCC;     /* 0xCC */
    u8 padD0[0xFC - 0xD0];
    u16 fFC;      /* 0xFC */
};

extern Sub *func_8012C194(Obj *o);
extern void func_8012CAE4(Obj *o);
extern void func_8001CB6C(Sub *a0, void *a1, s32 a2, s32 a3);
extern u8 D_80185DE8[];
extern u8 D_80185E58[];

void func_80144880(Obj *o) {
    Sub *s0 = func_8012C194(o);
    s32 v0;

    if (s0 == 0) {
        func_8012CAE4(o);
        return;
    }

    {
        u16 f36 = o->f64->f36;
        o->fCC = s0;
        o->fFC = f36;
    }

    if (o->f70 == 1) {
        func_8001CB6C(s0, D_80185DE8, 0x250, 0x1A0);
        v0 = 0x14;
    } else {
        func_8001CB6C(s0, D_80185E58, 0x300, 0x1D0);
        v0 = 0x6A;
    }

    s0->f27 = v0;
    s0->f4 = 0x60000000;
    s0->f10 = 0x400;
    s0->f8 = o->f64->f6;
    s0->fA = o->f64->fA;
    {
        u16 fe = o->f64->fE;
        s0->f1A = 0x4000;
        s0->f18 = 0x4000;
        s0->fC = fe;
    }

    o->f2++;
}
