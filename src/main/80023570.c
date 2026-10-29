/* Owner re-derivation from retail assembly (clean input set). */
/* Emits one flat TILE per entry of an object's rectangle list (last entry flagged), plus an optional blend-mode packet. */
#include "psx_types.h"

/* One 12-byte rectangle entry; last != 0 ends the list. */
typedef struct {
    u8 r;
    u8 g;
    u8 b;
    u8 last;
    u16 x;
    u16 y;
    u16 w;
    u16 h;
} RectEntry;

typedef struct {
    u8 pad0[4];
    u32 flags;   /* 0x04: bit 30 semi-transparent, bits 28-29 blend */
    u16 x;       /* 0x08 */
    u16 y;       /* 0x0A */
    u8 padC[0x20 - 0x0C];
    u32 rects;   /* 0x20: guest RectEntry * */
    u8 pad24[0x2C - 0x24];
    u16 depth;   /* 0x2C: ordering-table slot */
} RectObj;

/* libgpu P_TAG word: 24-bit next-packet address, 8-bit length. */
typedef struct {
    unsigned addr : 24;
    unsigned len : 8;
} PrimTag;

/* libgpu TILE layout. */
typedef struct {
    PrimTag tag;
    u8 r0;
    u8 g0;
    u8 b0;
    u8 code;
    s16 x0;
    s16 y0;
    s16 w;
    s16 h;
} Tile;

/* libgpu DR_MODE layout. */
typedef struct {
    PrimTag tag;
    u32 code[2];
} DrMode;

extern u32 D_800A5E60; /* guest primitive-buffer cursor */
extern PrimTag D_800A6610[][4096];
extern u16 D_800B9A02;
extern u32 func_80058B40(s32, s32, s32, s32);
extern void func_8005A600(void *, s32, s32, s32, s32);

void func_80023570(RectObj *o)
{
    RectEntry *e;
    PrimTag *ot;
    Tile *p;
    u32 flags;
    u16 ox;
    u16 oy;
    u16 depth;
    u8 code;
    DrMode *m;

    e = (RectEntry *)o->rects;
    ot = D_800A6610[D_800B9A02];
    p = (Tile *)D_800A5E60;
    flags = o->flags;
    ox = o->x;
    oy = o->y;
    depth = o->depth;
    code = 0x60;
    if (flags & 0x40000000) {
        code = 0x62;
    }
    do {
        p->code = code;
        *(u32 *)&p->tag = 0x03000000;
        p->r0 = e->r;
        p->g0 = e->g;
        p->b0 = e->b;
        p->x0 = ox + e->x;
        p->y0 = oy + e->y;
        p->w = e->w;
        p->h = e->h;
        D_800A5E60 += sizeof(Tile);
        p->tag.addr = ot[depth].addr;
        ot[depth].addr = (u32)p;
        p++;
    } while ((e++)->last == 0);
    if (flags & 0x40000000) {
        m = (DrMode *)D_800A5E60;
        D_800A5E60 += sizeof(DrMode);
        func_8005A600(m, 0, 0, func_80058B40(2, (flags >> 28) & 3, 0x280, 0) & 0xFFFF, 0);
        m->tag.addr = ot[depth].addr;
        ot[depth].addr = (u32)m;
    }
}
