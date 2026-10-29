#include "psx_types.h"

typedef struct { s32 w[8]; } M8;
typedef struct { u8 pad[0x34]; M8 m34; } Sub;
typedef struct { u8 pad[0x44]; u32 f44; } Z;
typedef struct { u8 pad[0x4C]; Z *f4C; } Y;
typedef struct Tgt {
    u16 id;
    u8 pad2[0x1E];
    Sub *f20;
} Tgt;
typedef struct {
    u8 pad0[2];
    u16 f2;
    u8 pad4[0xC];
    s16 h10;
    s16 h12;
    u8 pad14[8];
    s32 f1C;
    u8 pad20[0xC];
    s32 f2C;
    s32 f30;
    Tgt *f34;
    M8 m38;
} Obj;

extern s32 func_8005C49C();
extern void func_80146C3C(Obj *o);
extern void func_80147324(s32 a);
extern void func_801670E4(Obj *o, s32 a, s32 b, s32 c);

void func_80167714(Obj *o) {
    Tgt *t;
    s32 f;

    t = o->f34;
    if (t->id == 1) {
        if (o->h12 > o->h10) {
            if (o->f1C > 0) {
                o->h10 = func_8005C49C() & 0x30;
                o->f1C = -0x20;
            }
        } else if (o->f1C < 0) {
            o->h10 = (func_8005C49C() & 0x30) + 0x40;
            o->f1C = 0x20;
        }
        o->h12 = o->h12 + o->f1C;
        if (o->f2C & 8) {
            o->f30 = o->f30 + 0x80;
        } else {
            o->f30 = o->f30 - 0x40;
        }
        f = o->f2C;
        o->f2C = f + 1;
        if (!(f & 0xF)) {
            func_80147324(0x44E);
        }
        o->m38 = t->f20->m34;
        func_801670E4(o, 0, 0, 0);
        if (!(((Y *)o->f34)->f4C->f44 & 0x20)) {
            o->f2 = o->f2 - 1;
        }
    } else {
        func_80146C3C(o);
    }
}
