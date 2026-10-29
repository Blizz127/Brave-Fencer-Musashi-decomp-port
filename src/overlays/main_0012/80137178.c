#include "psx_types.h"
typedef struct { u32 w0; s16 a, b, c, d; u8 e, f, g; } Rec;
typedef struct { u32 w[5]; } Ent;
extern u16 D_800B9A02;
extern u8 D_800A6518[];
extern void func_80051DC8(Rec *r, Ent *e, s32 z);
void func_80137178(s16 x, s16 y) {
    Rec r;
    s16 x0;
    s16 x1;
    s16 yy;
    s16 ya;

    x0 = x - 3;
    yy = y - 3;
    x1 = x + 3;
    ya = yy;
    r.w0 = 0;
    r.e = 0;
    r.f = 0;
    r.g = 255;
    r.a = x0;
    r.b = yy;
    r.c = x1;
    r.d = yy;
    func_80051DC8(&r, &((Ent *)&D_800A6518)[D_800B9A02], 0);
    yy = y + 3;
    r.w0 = 0;
    r.e = 0;
    r.f = 0;
    r.g = 255;
    r.a = x0;
    r.b = yy;
    r.c = x1;
    r.d = ya;
    func_80051DC8(&r, &((Ent *)&D_800A6518)[D_800B9A02], 0);
    r.w0 = 0;
    r.e = 0;
    r.f = 0;
    r.g = 255;
    r.a = x0;
    r.b = yy;
    r.c = x1;
    r.d = yy;
    func_80051DC8(&r, &((Ent *)&D_800A6518)[D_800B9A02], 0);
}
