#include "psx_types.h"

typedef struct { u16 x; u16 y; u16 z; u16 w; } V8;
typedef struct {
    u16 id;
    u8 pad2[8];
    s16 fA;
    u8 padC[0x14];
    s32 f20;
    u8 pad24[0x34];
    s32 f58;
    u16 f5C;
    u8 pad5E[0x16];
    u8 f74;
    u8 f75;
    u8 pad76[0x10C - 0x76];
} Slot; /* 0x10C */

typedef struct {
    u8 pad0[6];
    u16 f6;
    u8 pad8[2];
    s16 fA;
    u8 padC[2];
    u16 fE;
    u8 pad10[0x68];
    u16 f78;
    u16 f7A;
    u16 f7C;
    u8 pad7E[0x1A];
    u16 f98;
    u16 f9A;
    u16 f9C;
    u8 pad9E[0xD6];
    Slot *f174;
} Obj;

extern s32 func_80135A4C(s32 shape, s32 kind, V8 *probe, V8 *out);
extern s32 func_80135888(s32 shape, s32 kind, V8 *probe, V8 *out);
extern s32 func_8014C918(Obj *o, s32 k);

s32 func_8014CD80(Obj *o, V8 *a, V8 *b) {
    Slot *p;
    s32 r;

    p = o->f174;
    if (p->id == 0 || (p->f5C & 0x400) == 0 || p->f58 == 0 || p->fA < o->fA) {
        if (p->id != 0) {
            p->f74 = 0;
        }
        o->f174 = 0;
        return 0;
    }
    a->x = o->f98;
    a->y = o->f9A - 3;
    a->z = o->f9C;
    if (p->f5C & 8) {
        b->y = a->y + 0xC;
        r = func_80135A4C(p->f20, p->f58, a, b);
        if (r != 0) {
            o->f6 = o->f78 = b->x;
            o->fA = o->f7A = b->y;
            o->fE = o->f7C = b->z;
        }
    } else {
        r = func_80135888(p->f20, p->f58, a, b);
    }
    if (r != 0) {
        *(s16 *)((u8 *)o + 0x16E) = func_8014C918(o, p->f75) & 0xFF;
        return 1;
    }
    p->f74 = 0;
    o->f174 = 0;
    return 0;
}
