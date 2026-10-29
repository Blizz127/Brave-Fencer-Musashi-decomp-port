/* Owner re-derivation from retail assembly (clean input set). */
/* Draws the 16x16 pointer sprite next to the first entry of the current window's position list (the window record D_80115134[D_8011511A], list at offset 0x18): when the mode D_8011512C is 1 the sprite is shifted by the blink offset D_80182798[D_80115116 & 7]. Links the sprite and a draw-mode packet (func_8005A600) into the current ordering table and returns the cursor advanced past them (cursor + 0x1C). */
#include "psx_types.h"

typedef struct {
    unsigned addr : 24;
    unsigned len : 8;
} PrimTag;

typedef struct {
    u32 ot;
    u8 pad04[0x10];
} Buf;

typedef struct {
    u32 w0;
    u32 w1;
    PrimTag tag;
} OtHead;

typedef struct {
    u16 idx;
} Cur;

typedef struct {
    PrimTag tag;
    u32 rgbc;
    s16 x;
    s16 y;
    u8 u;
    u8 v;
    u16 clut;
    u16 w;
    u16 h;
} Sprt;

typedef struct {
    u8 pad0[0x18];
    u32 f18;
    u32 pad1c;
} Win;

extern s16 D_8011511A;
extern u32 D_80115134;
extern s16 D_8011512C;
extern u16 D_80115116;
extern u16 D_80182798[];
extern Cur D_800B9A02;
extern Buf D_800AE7BC[];
extern void func_8005A600(u32 p, s32 b, s32 c, u16 d, s32 e);

u32 func_8017DD28(u32 p)
{
    u16 *pos;
    s32 off;
    s32 dx;

    pos = (u16 *)((Win *)D_80115134)[D_8011511A].f18;
    if (D_8011512C == 1) {
        off = D_80182798[D_80115116 & 7];
    } else {
        off = 0;
    }
    *(u32 *)p = 0x4000000;
    ((Sprt *)p)->u = 0x70;
    ((Sprt *)p)->v = 0x10;
    ((Sprt *)p)->rgbc = 0x64808080;
    ((Sprt *)p)->clut = 0x4056;
    dx = off - 0x60;
    ((Sprt *)p)->x = pos[0] + dx;
    ((Sprt *)p)->y = pos[1] - 2;
    ((Sprt *)p)->h = 0x10;
    ((Sprt *)p)->w = 0x10;
    ((PrimTag *)p)->addr = ((OtHead *)D_800AE7BC[D_800B9A02.idx].ot)->tag.addr;
    ((OtHead *)D_800AE7BC[D_800B9A02.idx].ot)->tag.addr = (u32)p;
    p += 0x14;
    func_8005A600(p, 0, 0, 0x15, 0);
    ((PrimTag *)p)->addr = ((OtHead *)D_800AE7BC[D_800B9A02.idx].ot)->tag.addr;
    ((OtHead *)D_800AE7BC[D_800B9A02.idx].ot)->tag.addr = (u32)p;
    return p + 8;
}
