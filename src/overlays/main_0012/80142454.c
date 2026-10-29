#include "psx_types.h"

typedef struct {
    u16 a, b, c, d, e, f, g, h;
    u32 i;
} P;

extern u16 D_80126B5E;
extern u16 D_80126B62;
extern u16 D_80126B66;
extern u8 *func_8012C51C(P *p, s32 x);

void func_80142454(u16 id) {
    P p;
    u8 *r;
    u16 *q;
    q = &D_80126B5E;
    p.d = 0x13;
    p.a = *q;
    p.b = D_80126B62;
    p.c = D_80126B66;
    p.e = 1;
    p.f = 0;
    p.h = 0;
    p.i = 0;
    r = func_8012C51C(&p, 0);
    if (r != 0) {
        *(u16 *)(r + 0x52) = id;
        *(u32 *)(r + 0x64) = (u32)q - 6;
    }
}
