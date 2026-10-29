/* Owner re-derivation from retail assembly (clean input set). */
/* Screen-sized grey semi-transparent quad (POLY_F4, subtractive blend), linked into OT slot depth of the
 * current buffer (or into the overlay OT when depth is 0), followed by a blend-mode packet. */
#include "psx_types.h"

typedef struct {
    unsigned addr : 24;
    unsigned len : 8;
} PrimTag;

/* libgpu POLY_F4 layout. */
typedef struct {
    PrimTag tag;
    u8 r0, g0, b0, code;
    s16 x0, y0;
    s16 x1, y1;
    s16 x2, y2;
    s16 x3, y3;
} PolyF4;

/* Per-buffer draw context (0x14 bytes). */
typedef struct {
    u32 pad0;
    u32 ot; /* 0x04: guest PrimTag * */
    u8 pad8[0x14 - 8];
} DrawCtx;

/* The ot words of the two context arrays (D_800A6518 + 4, D_800AE7B8 + 4). */
typedef struct {
    u32 ot; /* guest PrimTag * */
    u8 pad4[0x14 - 4];
} OtWord;

extern DrawCtx D_800A6518[];
extern OtWord D_800A651C[];
extern DrawCtx D_800AE7B8[];
extern OtWord D_800AE7BC[];
/* D_800AF630 viewed as a record; buf is the word the disassembly labels D_800B9A02. */
typedef struct {
    u8 pad0[0x18C];
    u16 scr_w;   /* 0x18C (D_800AF7BC) */
    u16 scr_h;   /* 0x18E (D_800AF7BE) */
    u8 pad190[0xA3D2 - 0x190];
    u16 buf;     /* 0xA3D2: current double-buffer index */
} GameState;

extern GameState D_800AF630;
extern void *func_80010A08(s32 size);
extern s32 func_80016638(DrawCtx *ctx, s32 idx, s32 mode);

/* Returns retail's leftover v0: func_80016638's result from the shared final call. */
s32 func_80016224(u8 grey, u16 depth)
{
    GameState *state = &D_800AF630;
    PolyF4 *p;
    s32 r;

    p = func_80010A08(sizeof(PolyF4));
    p->tag.len = 5;
    p->code = 0x2A;
    p->r0 = p->g0 = p->b0 = grey;
    p->x0 = p->x2 = -state->scr_w / 2;
    p->x1 = p->x3 = state->scr_w / 2;
    p->y0 = p->y1 = -state->scr_h / 2;
    p->y2 = p->y3 = state->scr_h / 2;
    if (depth != 0) {
        p->tag.addr = ((PrimTag *)D_800A651C[state->buf].ot)[depth].addr;
        ((PrimTag *)D_800A651C[state->buf].ot)[depth].addr = (u32)p;
        r = func_80016638(&D_800A6518[state->buf], depth, 2);
    } else {
        p->tag.addr = ((PrimTag *)D_800AE7BC[state->buf].ot)->addr;
        ((PrimTag *)D_800AE7BC[state->buf].ot)->addr = (u32)p;
        r = func_80016638(&D_800AE7B8[state->buf], 0, 2);
    }
    return r;
}
