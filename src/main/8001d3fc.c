/* Owner re-derivation from retail assembly (clean input set). */
/* Builds one textured POLY_FT4 per sprite part (the list ends after the part whose low flag byte is 0xFF) and links each into the ordering table. */
#include "psx_types.h"

typedef struct {
    u16 flags; /* low byte 0xFF ends the list; 0x100 mirrors u, 0x200 mirrors v */
    u8 w;
    u8 h;
    s16 u; /* texel x, in texture-page units before the depth shift */
    u16 v;
    s16 ox; /* screen offsets, read by func_8001D8C4 */
    s16 oy;
} Part;

typedef struct {
    unsigned addr : 24;
    unsigned len : 8;
} PrimTag;

typedef struct {
    PrimTag tag;
    u8 r0, g0, b0, code;
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
    u32 pad0;
    u32 flags; /* bits 24-25 colour depth, 28-29 blend mode, 30 semi-transparent, 6 raw texture */
    u8 pad8[0x18];
    u32 parts; /* guest Part * */
    u8 r, g, b;
    u8 clutRow;
    u16 tx; /* texture VRAM position */
    u16 ty;
    u16 depth; /* ordering-table index in the low 14 bits */
} Obj;

extern u32 D_800A5E60; /* guest pointer: next free primitive */
extern PrimTag D_800A6610[][4096];
extern u16 D_800B9A02;
extern void func_8001D70C(Obj *);
extern void func_8001D8C4(Obj *, Part *, PolyFT4 *);

void func_8001D3FC(Obj *obj)
{
    PolyFT4 *p;
    Part *part;
    PrimTag *ot;
    u32 flags;
    u32 idx;
    s32 tp;
    s32 shift;
    u16 pf;
    u16 x;
    u16 y;
    u16 tpage;
    u16 lo; /* first texel, u then v */
    u16 hi; /* last texel, clamped to 0xFF */
    s32 yw;

    idx = obj->depth & 0x3FFF;
    if (idx < 0x1000) {
        flags = obj->flags;
        part = (Part *)obj->parts;
        tp = (flags >> 24) & 3;
        shift = 2 - tp;
        ot = &D_800A6610[D_800B9A02][idx];
        func_8001D70C(obj);
        do {
            p = (PolyFT4 *)D_800A5E60;
            pf = part->flags;
            x = obj->tx + (part->u >> shift);
            y = obj->ty + part->v;
            D_800A5E60 += 0x28; /* sizeof(PolyFT4): the lane bars sizeof (host widths differ) */
            p->tag.len = 9;
            p->code = 0x2C;
            if (flags & 0x40000000) {
                p->code = 0x2E;
                tpage = (tp << 7) | ((flags >> 23) & 0x60) | ((y & 0x100) >> 4) | ((x & 0x3C0) >> 6) | ((y & 0x200) << 2);
            } else {
                tpage = (tp << 7) | (((y & 0x100) >> 4) | 0x20) | ((x & 0x3C0) >> 6) | ((y & 0x200) << 2);
            }
            p->tpage = tpage;
            p->code |= (flags & 0x40) >> 6;
            lo = (x - ((tpage & 0xF) << 6)) << shift;
            hi = lo + part->w - 1;
            if (hi >= 0x100) {
                hi = 0xFF;
            }
            if (pf & 0x100) {
                p->u1 = p->u3 = lo;
                p->u0 = p->u2 = hi;
            } else {
                p->u0 = p->u2 = lo;
                p->u1 = p->u3 = hi;
            }
            yw = y;
            if (tpage & 0x10) {
                lo = yw - 0x100;
            } else {
                lo = yw;
            }
            hi = lo + part->h - 1;
            if (hi >= 0x100) {
                hi = 0xFF;
            }
            if (pf & 0x200) {
                p->v2 = p->v3 = lo;
                p->v0 = p->v1 = hi;
            } else {
                p->v0 = p->v1 = lo;
                p->v2 = p->v3 = hi;
            }
            {
                /* CLUT y = 0x100 + clutRow; x = 0x160 when clutRow < 0xE0, else 0x100. Kept as if/else: a
                 * conditional operator becomes a host select, and the lane's cycle map needs the branch
                 * that retail has at 8001D654. */
                s32 row = (obj->clutRow + 0x100) << 6;
                s32 clut;
                if (obj->clutRow < 0xE0) {
                    clut = row | 0x16;
                } else {
                    clut = row | 0x10;
                }
                p->clut = clut;
            }
            p->r0 = obj->r;
            p->g0 = obj->g;
            p->b0 = obj->b;
            func_8001D8C4(obj, part, p);
            p->tag.addr = ot->addr;
            ot->addr = (u32)p;
            part++;
        } while ((pf & 0xFF) != 0xFF);
    }
}
