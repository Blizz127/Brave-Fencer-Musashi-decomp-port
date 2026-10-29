/* func_801415C0 [801415C0,8014168C) - owner re-derivation from retail assembly (clean input set). */
/* Swaps two slots in both tables: the byte entries at a+0x2F / b+0x2F (func_800291B4 / func_800291A0)
 * and the halfword entries at a*2+0x14 / b*2+0x14 (func_800291DC / func_800291C8). */
#include "psx_types.h"

/* callers read v0: no (callers 8013F600/8013F650 ignore it; retail sets none).
 * Prototypes here use s32 returns: the owner bodies declare func_800291B4 as u8 and
 * func_800291DC as s16; the retail caller masks/extends at the use instead. */
extern s32 func_800291B4(s32 id);
extern void func_800291A0(s32 id, s32 val);
extern s32 func_800291DC(s32 i);
extern void func_800291C8(s32 i, s16 v);

void func_801415C0(s16 a, s16 b)
{
    s32 t;
    s32 y;
    s32 v;

    /* t carries the first saved value of both halves (one pseudo, one s-reg) */
    t = func_800291B4(a + 0x2F) & 0xFF;
    y = func_800291B4(b + 0x2F) & 0xFF;
    func_800291A0(a + 0x2F, y);
    func_800291A0(b + 0x2F, t);
    t = func_800291DC(a * 2 + 0x14);
    v = func_800291DC(b * 2 + 0x14);
    func_800291C8(a * 2 + 0x14, v);
    func_800291C8(b * 2 + 0x14, t);
}
