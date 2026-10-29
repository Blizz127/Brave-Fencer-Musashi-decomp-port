/* func_80171520 [80171520,80171628) - owner re-derivation from retail assembly (clean input set).
 * Copies three 16-byte vectors into the record at D_8011F738, clears their colour bytes and links
 * the record into o->f20. */
#include "psx_types.h"

typedef struct {
    u32 w0, w1, w2;
    u8 r, g, b, code;
} V;

typedef struct {
    V a;
    V b;
    V c;
} Q;

typedef struct {
    u8 p0[0x2C];
    u16 f2C;
    u8 p2E[0x52];
    u32 f80;
} S;

typedef struct {
    u8 p0[0x20];
    S *f20;
} O;

/* callers read v0: none-direct (no jal caller); void. D_8011F738 holds a guest address (pointer). */
extern void func_801719A4(O *o, s32 a);
extern void func_80171990(O *o);
extern void func_8017162C(O *o);
extern u32 D_8011F738;
extern V D_800A5E88;
extern V D_800A5E98;
extern V D_800A5EA8;

void func_80171520(O *o) {
    Q *p;

    func_801719A4(o, 1);
    p = (Q *)D_8011F738;
    p->a = D_800A5E88;
    p->b = D_800A5E98;
    p->c = D_800A5EA8;
    p->a.r = p->a.g = p->a.b = 0;
    p->b.r = p->b.g = p->b.b = 0;
    p->c.r = p->c.g = p->c.b = 0;
    o->f20->f2C |= 0x20;
    o->f20->f80 = (u32)p;
    func_80171990(o);
    func_8017162C(o);
}
