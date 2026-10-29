/* Owner re-derivation from retail assembly (clean input set). */
/* Draws a rectangle outline as four LINE_F2 packets in OT slot depth of the current buffer;
 * blend modes 0-3 make the lines semi-transparent and add a blend-mode packet. */
#include "psx_types.h"

typedef struct {
    u16 x;
    u16 y;
} Point;

typedef struct {
    unsigned addr : 24;
    unsigned len : 8;
} PrimTag;

/* libgpu LINE_F2 layout. */
typedef struct {
    PrimTag tag;
    u8 r0, g0, b0, code;
    s16 x0, y0;
    s16 x1, y1;
} LineF2;

typedef struct {
    u32 pad0;
    u32 ot; /* 0x04: guest PrimTag * */
    u8 pad8[0x14 - 8];
} DrawCtx;

typedef struct {
    u32 ot; /* guest PrimTag * */
    u8 pad4[0x14 - 4];
} OtWord;

extern u8 D_800AF630[];
extern DrawCtx D_800A6518[];
extern OtWord D_800A651C[];
extern void *func_80010A08(s32 size);
extern void func_80016638(DrawCtx *ctx, s32 idx, s32 mode);

void func_80015F04(s16 x, s16 y, s16 w, s16 h, u8 r, u8 g, u8 b, u16 depth, u16 mode)
{
    register u8 *state = D_800AF630;
    Point pts[4];
    LineF2 *p;
    s32 i;

    pts[0].x = x;
    pts[0].y = y;
    pts[1].x = x + w;
    pts[1].y = y;
    pts[2].x = x + w;
    pts[2].y = y + h;
    pts[3].x = x;
    pts[3].y = y + h;
    p = func_80010A08(sizeof(LineF2) * 4);
    for (i = 0; i < 4; i++, p++) {
        p->tag.len = 3;
        p->code = 0x40;
        if (mode < 4) {
            p->code = 0x42;
        }
        p->r0 = r;
        p->g0 = g;
        p->b0 = b;
        p->x0 = pts[i].x;
        p->y0 = pts[i].y;
        /* the last line reads one point past the array; it is patched below */
        p->x1 = pts[i + 1].x;
        p->y1 = pts[i + 1].y;
        p->tag.addr = ((PrimTag *)D_800A651C[*(u16 *)(state + 0xA3D2)].ot)[depth].addr;
        ((PrimTag *)D_800A651C[*(u16 *)(state + 0xA3D2)].ot)[depth].addr = (u32)p;
    }
    p[-1].x1 = pts[0].x;
    p[-1].y1 = pts[0].y;
    if (mode < 4) {
        func_80016638(&D_800A6518[*(u16 *)(state + 0xA3D2)], depth, mode);
    }
}
