/* Owner re-derivation from retail assembly (clean input set). */
/* Draws the list marker: allocates two 0x24-byte quads plus an 8-byte draw-mode packet, fills the quads and links all three into the ordering table at slot +0x1A. */
#include "psx_types.h"

typedef struct {
    u32 addr : 24;
    u8 len;
} Tag;

typedef struct {
    Tag tag;
    u8 pad[0x20];
} Quad;

typedef struct {
    Tag tag;
    u32 code;
} Mode;

typedef struct {
    u8 p0[4];
    Tag *ot;
} T;

typedef struct {
    u8 p0[0x12];
    u16 f12;
    u8 p14[2];
    u16 f16;
    u8 p18[2];
    u16 f1A;
    u8 p1C[3];
    u8 f1F;
    u8 p20;
    u8 f21;
    u8 p22[0xC];
    u16 f2E;
    u8 p30[2];
    u16 f32;
} O;

extern void *func_80010A08(s32 size);
extern void func_80137FD8(void *o, u8 *c, void *p, s32 y);

void func_80137DD4(O *o, u8 *c, T *t) {
    Quad *p;
    Mode *m;
    s16 d;
    s16 a;
    s32 k;
    s16 y;
    d = o->f12 - o->f16;
    a = d;
    if (d < 0) {
        a = d + 1;
        a += o->f2E;
    }
    k = (s16)(a - o->f21 + (s16)((s8)o->f1F + 1));
    y = o->f32 + k * 14;
    p = func_80010A08(0x48);
    c[3] = 0;
    func_80137FD8(o, c, p, y);
    p->tag.addr = t->ot[o->f1A].addr;
    t->ot[o->f1A].addr = (u32)p;
    p++;
    c[3] = 1;
    func_80137FD8(o, c, p, y);
    p->tag.addr = t->ot[o->f1A].addr;
    t->ot[o->f1A].addr = (u32)p;
    m = func_80010A08(8);
    m->tag.len = 1;
    m->code = 0xE100000A;
    m->tag.addr = t->ot[o->f1A].addr;
    t->ot[o->f1A].addr = (u32)m;
}
