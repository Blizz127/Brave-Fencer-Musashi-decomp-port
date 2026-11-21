/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern u16 D_80126B5E, D_80126B62, D_80126B66;
extern u16 D_80126940;
extern s16 D_80126944;
extern s16 D_80126942;

/* The historical compiler emits the game's aligned word and halfword
 * stores. The host model may receive byte storage at any alignment. */
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define write_word(p, v) (*(u32 *)(p) = (v))
#define write_half(p, v) (*(u16 *)(p) = (v))
#else
static void write_half(u8 *p, u16 v)
{
    p[0] = (u8)v;
    p[1] = (u8)(v >> 8);
}

static void write_word(u8 *p, u32 v)
{
    write_half(p, (u16)v);
    write_half(p + 2, (u16)(v >> 16));
}
#endif

/* Returns retail's leftover v0: the u16 D_80126B5E (zero-extended). */
s32 func_8012A048(u8 *o, u32 a, u8 b) {
    s32 r;
    write_word(o, a);
    o[4] = b;
    write_word(o + 0x9C, 0);
    write_half(o + 0xA0, 0);
    write_half(o + 0xA2, 0);
    r = D_80126B5E;
    D_80126940 = r;
    D_80126942 = (s16)D_80126B62;
    D_80126944 = (s16)D_80126B66;
    return r;
}
