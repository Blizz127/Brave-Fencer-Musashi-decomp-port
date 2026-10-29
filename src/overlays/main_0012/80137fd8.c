/* func_80137FD8 [80137FD8,801380E0) - owner re-derivation from retail assembly (clean input set).
 * Fills a 0x24-byte gouraud quad primitive (code 0x3A). */
#include "psx_types.h"

typedef struct {
    u8 p0[3];
    u8 len;
    u8 r0, g0, b0, code;
    s16 x0, y0;
    u8 r1, g1, b1, pD;
    s16 x1, y1;
    u8 r2, g2, b2, p17;
    s16 x2, y2;
    u8 r3, g3, b3, p1F;
    s16 x3, y3;
} Prim;


typedef struct {
    u8 p0[8];
    u32 f8;
    u8 pC[0x24];
    u16 f30;
    u16 f32;
    u16 f34;
} O;

/* callers read v0: no (2 direct callers, neither reads v0); void. */
void func_80137FD8(O *o, u8 *c, Prim *p, s32 y) {
    p->len = 8;
    p->code = 0x3A;
    p->r0 = c[0];
    p->g0 = c[1];
    p->b0 = c[2];
    p->r1 = c[0];
    p->g1 = c[1];
    p->b1 = c[2];
    p->r2 = 0xFF;
    p->g2 = 0xFF;
    p->b2 = 0xFF;
    p->r3 = 0xFF;
    p->g3 = 0xFF;
    p->b3 = 0xFF;
    p->x0 = p->x2 = o->f30;
    if (o->f8 & 0x2000) {
        s32 a = o->f30 + o->f34 - 12;
        p->x3 = a;
        p->x1 = a;
    } else {
        s32 b = o->f30 + o->f34;
        p->x3 = b;
        p->x1 = b;
    }
    if (c[3] == 0) {
        s32 a = y + 5;
        p->y1 = y;
        p->y0 = y;
        p->y3 = a;
        p->y2 = a;
    } else {
        s32 w = y + 12;
        s32 b = y + 5;
        p->y0 = p->y1 = w;
        p->y2 = p->y3 = b;
    }
}
