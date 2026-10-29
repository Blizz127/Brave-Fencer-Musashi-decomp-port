/* Owner re-derivation from retail assembly (clean input set). tx/ty are u16 locals: their HI-mode sets keep sched1
 * from boosting them as single-set "birthing" leaves, so the s16 copy x = x0 stays after them and tx/ty read x0. */
/* func_8001BC6C: queue a grey-tinted textured rect (code 0x64, 0x18 bytes) from the (x, y, w, h) halfword record
 * `r` into the current ordering table; x >= 0x80 selects the next texture page. */
#include "psx_types.h"

typedef struct {
    unsigned addr : 24;
    unsigned len : 8;
} PrimTag;

typedef struct {
    PrimTag tag;
    u32 mode;
    u32 rgb;
    u32 xy;
    u32 uv;
    u32 wh;
} Rect;

extern Rect *D_800A5E60;
extern PrimTag D_800A6610[][4096];
extern u16 D_800B9A02;

void func_8001BC6C(u16 *r, u32 c) {
    Rect *p;
    PrimTag *ot;
    u16 x0, y0, w0, h0;
    s16 x;
    s32 page;
    u32 rgb;
    u16 tx, ty;

    page = 0x100;
    x0 = r[0];
    y0 = r[1];
    w0 = r[2];
    h0 = r[3];
    p = D_800A5E60;
    tx = x0 - 0xA0;
    ty = y0 - 0x78;
    x = x0;
    rgb = (c << 16) | 0x64000000 | (c << 8) | c;
    ot = D_800A6610[D_800B9A02];
    if (x >= 0x80) {
        x -= 0x80;
        page = 0x102;
    }
    p->tag.len = 5;
    p->mode = 0xE1000000 | page;
    ((u8 *)p)[0xB] = 0x64;
    p->rgb = rgb;
    p->xy = (ty << 16) | (tx & 0xFFFF);
    p->uv = ((s16)y0 << 8) | x;
    p->wh = (h0 << 16) | (s16)w0;
    D_800A5E60 = p + 1;
    p->tag.addr = ot[1].addr;
    ot[1].addr = (u32)p;
}
