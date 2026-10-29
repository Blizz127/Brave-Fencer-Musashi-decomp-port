/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the sc01_0005 twin at this address (W16). */
/* Draws a vertical tile strip: one 12-pixel slice per entry of a ring buffer (the first slice
 * trimmed by `trim`), then a closing slice that may be split in two when it is wider than 252. */
#include "psx_types.h"

/* Packet built on the stack; fields as read by the sprite submit routine. */
typedef struct {
    s32 tag;
    u16 x;
    u16 y;
    u16 w;
    s16 h;
    u16 clut;
    u8 u;
    u8 v;
    u16 pad10;
    u16 pad12;
    u8 r;
    u8 g;
    u8 b;
    u8 spare[0x11];
} Prim;

/* Strip descriptor. */
typedef struct {
    u8 pad0[0x16];
    u16 start;  /* 0x16 */
    u8 pad18[2];
    u16 depth;  /* 0x1A */
    u8 pad1c[0x12];
    s16 count;  /* 0x2E */
    u16 x;      /* 0x30 */
    u16 y;      /* 0x32 */
    s16 w;      /* 0x34 */
    u8 pad36[2];
    s16 page;   /* 0x38 */
    s16 vbase;  /* 0x3A (low byte used as base row) */
    u16 pad3c;  /* 0x3C */
    u16 pad3e;  /* 0x3E */
} Strip;

extern s32 func_80058B40(s32 tp, s32 abr, s32 x, s32 y);
extern void func_80052460(Prim *p, u32 arg, u16 depth);

void func_801392FC(Strip *o, s32 trim, u32 ctx)
{
    Prim p;
    s32 i;
    s32 idx;

    p.tag = 0;
    p.clut = func_80058B40(0, 1, o->page & 0xFFC0, o->vbase & 0xFF00);
    p.pad10 = o->pad3c;
    p.pad12 = o->pad3e;
    p.b = 0x80;
    p.g = 0x80;
    p.r = 0x80;
    for (i = 0; i < o->count; i++) {
        idx = (o->start + i) % (o->count + 1);
        p.x = o->x;
        if (i != 0) {
            p.y = o->y + i * 12 - trim + i * 2;
        } else {
            p.y = o->y;
        }
        if (i == 0) {
            p.h = 12 - trim;
        } else {
            p.h = 12;
        }
        p.u = 0;
        if (i != 0) {
            p.v = idx * 12 + (u8)o->vbase;
        } else {
            p.v = idx * 12 + (u8)o->vbase + trim;
        }
        p.w = o->w;
        func_80052460(&p, ctx, o->depth);
    }
    if (trim != 0) {
        idx = (o->start + i) % (o->count + 1);
        p.x = o->x;
        p.y = o->y + i * 12 - trim + i * 2;
        p.h = trim - 2;
        p.u = 0;
        p.v = idx * 12 + (u8)o->vbase;
        if (o->w >= 0xFD) {
            p.w = 0x100;
            func_80052460(&p, ctx, o->depth);
            p.x = o->x + p.w;
            p.w = 0x20;
            p.u = idx << 5;
            p.v = (o->count + 1) * 12 + (u8)o->vbase;
            func_80052460(&p, ctx, o->depth);
        } else {
            p.w = o->w;
            func_80052460(&p, ctx, o->depth);
        }
    }
}
