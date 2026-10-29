#include "psx_types.h"

typedef struct { s32 w[8]; } M8;
typedef struct {
    u8 pad0[0x10];
    u16 h10;
    u16 h12;
    u16 h14;
    u8 pad16[0x1E];
    M8 m34;
} Sub;
typedef struct {
    u8 pad0[0x10];
    u16 h10;
    u16 h12;
    u16 h14;
    u16 pad16;
    u16 h18;
    u16 h1A;
    u16 h1C;
    u8 pad1E[2];
    u8 *f20;
    u8 pad24[8];
    u16 f2C;
    u8 pad2E[6];
    M8 m34;
} R;
typedef struct {
    u8 pad0[0x20];
    Sub *f20;
    u8 pad24[0x20];
    u32 f44;
    u8 pad48[0xB8];
    u16 h100;
    u16 h102;
    u16 h104;
} Q;
typedef struct {
    u8 pad0[0x20];
    R *f20;
    u8 pad24[0x28];
    Q *f4C;
    u8 pad50[0x10];
    u16 h60;
    u16 h62;
    u16 h64;
} Obj;

extern u8 D_800636C8[];
extern u8 D_800DE5A0[];
extern R *func_8014659C();
extern void func_8001C214(R *r, u8 *a);
extern void func_80149374(Q *q, void *p);
extern void func_80147324(s32 a);
extern void func_80146E90(Obj *o, s32 a);
extern s32 func_80146A6C(s32 a, Obj *o, s32 b, s32 c, s32 d, s32 e, s32 f);
extern void func_80146DE8(Obj *o, s32 a, s32 b, s32 c);
extern void func_80146CA0(Obj *o);

void func_80163A94(Obj *o) {
    Q *q;
    R *r;

    q = o->f4C;
    q->f44 = q->f44 | 4;
    r = func_8014659C();
    o->f20 = r;
    if (r != 0) {
        func_8001C214(r, D_800DE5A0);
        r->f20 = D_800636C8;
        r->h18 = r->h1A = r->h1C = 0x1800;
        r->f2C = r->f2C | 0x10;
        func_80149374(q, (u8 *)o + 4);
        o->f20->m34 = q->f20->m34;
        r->h10 = q->f20->h10;
        r->h12 = q->f20->h12;
        r->h14 = q->f20->h14;
        o->h60 = q->h100;
        o->h62 = q->h102;
        o->h64 = q->h104;
        func_80147324(0x448);
        func_80146E90(o, 0x10);
        func_80146A6C(0xD, o, 0, 0, 0, 0, 0);
        func_80146DE8(o, 0, 0, 0xFFE00000);
        func_80146CA0(o);
    }
}
