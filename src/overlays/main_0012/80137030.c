#include "psx_types.h"
typedef struct { u32 w0; s16 a, b, c, d; u8 e, f, g; } Rec;
typedef struct { u32 w[5]; } Ent;
extern u16 D_800B9A02;
extern u8 D_800A6518[];
extern void func_80051DC8(Rec *r, Ent *e, s32 z);
void func_80137030(s16 x, s16 y) {
    Rec r;
    s16 xd;
    s16 yy;

    xd = x - 3;
    yy = y - 3;
    r.w0 = 0;
    r.e = 0;
    r.f = 255;
    r.g = 0;
    r.a = xd;
    r.b = yy;
    r.c = x;
    r.d = y;
    func_80051DC8(&r, &((Ent *)&D_800A6518)[D_800B9A02], 0);
    xd = x + 3;
    r.w0 = 0;
    r.e = 0;
    r.f = 255;
    r.g = 0;
    r.a = xd;
    r.b = yy;
    r.c = x;
    r.d = y;
    func_80051DC8(&r, &((Ent *)&D_800A6518)[D_800B9A02], 0);
    yy = y + 3;
    r.w0 = 0;
    r.e = 0;
    r.f = 255;
    r.g = 0;
    r.a = x;
    r.b = yy;
    r.c = x;
    r.d = y;
    func_80051DC8(&r, &((Ent *)&D_800A6518)[D_800B9A02], 0);
}
