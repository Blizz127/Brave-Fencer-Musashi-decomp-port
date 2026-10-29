/* Owner re-derivation from retail assembly (clean input set). */
/* Full-screen grey semi-transparent fade quad (POLY_F4), linked into OT slot depth of the current buffer
 * (or into the overlay OT when depth is 0), followed by a blend-mode packet. */
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
    u8 pad0[0xA3D2];
    u16 buf; /* 0xA3D2: current double-buffer index */
} GameState;

extern GameState D_800AF630;
extern void *func_80010A08(s32 size);
extern void func_80016638(DrawCtx *ctx, s32 idx, s32 mode);

void func_80016450(u8 grey, u16 depth)
{
    GameState *state = &D_800AF630;
    PolyF4 *p;

    p = func_80010A08(sizeof(PolyF4));
    p->tag.len = 5;
    p->code = 0x2A;
    p->x0 = p->x2 = -160;
    p->x1 = p->x3 = 160;
    p->y0 = p->y1 = -120;
    p->r0 = p->g0 = p->b0 = grey;
    p->y2 = p->y3 = 120;
    if (depth != 0) {
        p->tag.addr = ((PrimTag *)D_800A651C[state->buf].ot)[depth].addr;
        ((PrimTag *)D_800A651C[state->buf].ot)[depth].addr = (u32)p;
        func_80016638(&D_800A6518[state->buf], depth, 1);
    } else {
        p->tag.addr = ((PrimTag *)D_800AE7BC[state->buf].ot)->addr;
        ((PrimTag *)D_800AE7BC[state->buf].ot)->addr = (u32)p;
        func_80016638(&D_800AE7B8[state->buf], 0, 1);
    }
}
