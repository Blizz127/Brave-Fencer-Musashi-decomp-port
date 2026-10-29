#include "psx_types.h"

typedef struct {
    u8 pad0[0x5C];
    s32 f5C;
    u8 pad60[4];
    s32 f64;
    s32 f68;
    u8 pad6C[4];
    s32 f70;
} Cam;
typedef struct { u8 pad[0x12]; s16 f12; } Sub;
typedef struct {
    u8 pad0[0x20];
    Sub *p20;
    u8 pad24[0x85];
    u8 fA9;
    u8 padAA[4];
    u16 fAE;
} Obj;

extern u32 D_801151D4;
extern s32 func_8004CFEC(s32 dx, s32 dy);
extern s32 func_80047948(s32 a);
extern s32 func_8004787C(s32 a);

void func_80172170(Obj *o, u8 a) {
    s32 s0;
    s32 t1;
    s32 h1;
    s32 x;
    s32 y;
    Cam *c;

    c = (Cam *)D_801151D4;
    o->fA9 = 0x73;
    s0 = func_8004CFEC(c->f68 - c->f5C, c->f70 - c->f64);
    s0 = (s0 + 0x800) & 0xFFF;
    s0 = (o->p20->f12 - s0) & 0xFF8;
    if (s0 == 0x400 || s0 == 0xC00) {
        x = 0x80;
    } else {
        t1 = func_80047948(s0);
        h1 = a;
        x = (-(h1 >> 1) * t1) / 4096 + 0x80;
    }
    if (x < 0) {
        x = 0;
    }
    if (x >= 0x100) {
        x = 0xFF;
    }
    if (s0 == 0 || s0 == 0x800) {
        y = 0x80;
    } else {
        t1 = func_8004787C(s0);
        h1 = a;
        y = ((h1 >> 1) * t1) / 4096 + 0x80;
    }
    if (y < 0) {
        y = 0;
    }
    if (y >= 0x100) {
        y = 0xFF;
    }
    o->fAE = (y & 0xFF) | (x << 8);
}
