/* func_80139680 [80139680,801397A8) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u32 tag;
    s16 x;
    s16 y;
    s16 w;
    s16 h;
    u16 clut;
    u8 u;
    u8 v;
    s16 f20;
    s16 f22;
    u8 r, g, b;
    u8 p27;
} Prim;

typedef struct {
    u8 p0[0x12];
    u16 f12;
    u16 f14;
    u16 f16;
    u8 p18[2];
    u16 f1A;
    u8 p1C[0x14];
    s16 f30;
    u16 f32;
    u8 p34[8];
    u16 f3C;
    u16 f3E;
} O;

/* callers read v0: no (2 direct callers, neither reads v0); void.
 */
extern s32 func_80058B40(s32 a, s32 b, s32 c, s32 d);
extern void func_80052460(Prim *p, s32 a, s32 b);

void func_80139680(O *o, s32 a) {
    Prim s;
    s32 pad[4]; /* unreferenced: retail frame keeps 16 bytes above the primitive */
    s32 d;
    s32 y;
    s32 bx;
    s32 v;

    d = o->f12 - o->f16;
    if (d < 0) {
        d += 4;
    }
    s.tag = 0;
    s.clut = func_80058B40(0, 1, 0x1C0, 0x100);
    s.f20 = o->f3C;
    s.f22 = o->f3E;
    s.r = s.g = s.b = 0x80;
    bx = o->f30;
    if (o->f14 < 0x24) {
        v = o->f14 * 6 + bx;
    } else {
        v = bx + 0xD2;
    }
    s.x = v;
    y = o->f32 + d * 12;
    s.y = y;
    if (d != 0) {
        s.y += d * 2;
    }
    s.w = 0xC;
    s.h = 0xC;
    s.u = 0xF0;
    s.v = 0;
    func_80052460(&s, a, o->f1A);
}
