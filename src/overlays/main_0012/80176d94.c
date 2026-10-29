/* Owner re-derivation from retail assembly (clean input set). */
/* Builds the gauge widget: four 0x14-byte packets, two sprite strips from func_801777BC, then three 0x14-byte tiles (frame, shadow, bar) sized from D_8011F82A/82C; returns the next free packet.
 * callers read v0: yes (80175B48 passes it on), so Prim *. The tile words are written as raw
 * non-struct stores so the D_8011F82A read stays after the first tile and D_8011F82C is read first. */
#include "psx_types.h"

typedef struct {
    u32 tag;
    u32 cmd;
    u32 xy;
    u32 wh;
    u32 pad;
} Prim;

extern u16 D_8011F82A;
extern u16 D_8011F82C;
extern Prim *func_80177784(Prim *p, s32 a, s32 b);
extern Prim *func_801777BC(Prim *p, u32 a1, u32 x, s32 y, s16 n, s16 c, s32 k);
extern s32 func_801783D0(s32 n, s32 shift);

Prim *func_80176D94(Prim *p, s32 x, s16 y) {
    u32 hy = (u32)y << 16;
    u32 xy1;
    u32 xy3;
    u32 w;
    s32 h0;
    p = func_80177784(p, hy | (x & 0xFFFF), 0x6B565040);
    p = func_80177784(p, hy | ((x + 8) & 0xFFFF), 0x6B565808);
    p = func_80177784(p, hy | ((x + 0x10) & 0xFFFF), 0x6B564060);
    p = func_80177784(p, hy | ((x + 0x30) & 0xFFFF), 0x6B563858);
    p = func_801777BC(p, (s16)func_801783D0(D_8011F82C, 4), (s16)(x + 0x18), y, 3, 0xAD, 0xFF);
    p = func_801777BC(p, (s16)func_801783D0(D_8011F82A, 4), (s16)(x + 0x38), y, 3, 0xAD, 0xFF);
    h0 = (((s32)D_8011F82C * 20710) >> 16) | 0x20000;
    xy1 = ((y + 0xA) << 16) | ((x + 1) & 0xFFFF);
    xy3 = ((y + 9) << 16) | (u16)x;
    *(u32 *)p = (((u32)p - 0x14) & 0xFFFFFF) | 0x3000000;
    *(u32 *)((u8 *)p + 0x4) = 0x6000DCFF;
    *(u32 *)((u8 *)p + 0x8) = xy1;
    *(u32 *)((u8 *)p + 0xC) = h0;
    w = ((s32)D_8011F82A * 20710) >> 16;
    *(u32 *)((u8 *)p + 0x14) = ((u32)p & 0xFFFFFF) | 0x3000000;
    *(u32 *)((u8 *)p + 0x18) = 0x600000FF;
    *(u32 *)((u8 *)p + 0x1C) = xy1;
    *(u32 *)((u8 *)p + 0x20) = w | 0x20000;
    *(u32 *)((u8 *)p + 0x28) = (((u32)p + 0x14) & 0xFFFFFF) | 0x3000000;
    *(u32 *)((u8 *)p + 0x2C) = 0x60000000;
    *(u32 *)((u8 *)p + 0x30) = xy3;
    *(u32 *)((u8 *)p + 0x34) = (w + 2) | 0x40000;
    return p + 3;
}
