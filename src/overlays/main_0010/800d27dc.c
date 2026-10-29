/* Owner re-derivation from retail assembly (clean input set). */
/* Builds one textured POLY_FT4 per 6- or 10-halfword source record (x, y, w, h, u, v [, clut, tpage, du, dv]) and links each into the current ordering table. Returns the next free primitive. */
#include "psx_types.h"

typedef struct {
    unsigned addr : 24;
    unsigned len : 8;
} PrimTag;

typedef struct {
    u32 tag;
    u32 rgbc;
    s16 x0, y0;
    u8 u0, v0;
    u16 clut;
    s16 x1, y1;
    u8 u1, v1;
    u16 tpage;
    s16 x2, y2;
    u8 u2, v2;
    u16 pad2;
    s16 x3, y3;
    u8 u3, v3;
    u16 pad3;
} PolyFT4;

typedef struct {
    u32 ot; /* guest pointer to the table's head words */
    u8 pad04[0x10];
} Buf80175820; /* 0x14 */

typedef struct {
    u32 w0;
    u32 w1;
    PrimTag tag;
} OtHead;

typedef struct {
    u16 idx;
} Cur80175820;

extern Cur80175820 D_800B9A02;
extern Buf80175820 D_800AE7BC[];

PolyFT4 *func_800D27DC(u32 flags, PolyFT4 *p, u16 *src, s16 n, u16 xoff)
{
    u32 rgbc;
    s16 i;
    s32 dx;
    u16 xs;
    u16 ys;
    u16 us;
    u16 vs;
    s32 k;
    s32 f2;
    s32 x;
    u16 y;
    s32 w;
    u16 h;
    u16 u;
    u16 v;
    u16 du;
    u16 dv;

    dx = xoff;
    rgbc = 0x2C808080;
    if (flags & 1) {
        rgbc = 0x2C585858;
    }
    i = 0;
    if (n > 0) {
        f2 = flags & 2;
        do {
            p->tag = 0x09000000;
            p->rgbc = rgbc;
            x = *src++ + dx;
            p->x2 = x;
            p->x0 = x;
            y = *src++;
            p->y1 = y;
            p->y0 = y;
            w = *src++;
            h = *src++;
            u = *src++;
            p->u0 = p->u2 = u;
            v = *src++;
            p->v0 = p->v1 = v;
            if (!f2) {
                dv = 8;
                p->clut = 0x4056;
                k = i & 7;
                p->tpage = 0x15;
                du = 8;
                if (k >= 4) {
                    if (k < 6) {
                        du = 7;
                    } else if (k < 8) {
                        dv = 7;
                    }
                }
            } else {
                p->clut = *src++;
                p->tpage = *src++;
                du = *src++;
                dv = *src++;
            }
            xs = x + w;
            ys = y + h;
            us = u + du;
            vs = v + dv;
            p->x3 = xs;
            p->x1 = xs;
            p->y3 = ys;
            p->y2 = ys;
            p->u3 = us;
            p->u1 = us;
            p->v3 = vs;
            p->v2 = vs;
            ((PrimTag *)p)->addr = ((OtHead *)D_800AE7BC[D_800B9A02.idx].ot)->tag.addr;
            ((OtHead *)D_800AE7BC[D_800B9A02.idx].ot)->tag.addr = (u32)p;
            p++;
            i++;
        } while (i < n);
    }
    return p;
}
