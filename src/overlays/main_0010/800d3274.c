/* Owner re-derivation from retail assembly (clean input set). */
/* If o->f198 is set, replaces it with func_8013767C(o->f198) and advances the byte at
 * o+0x214 by 1; else advances the byte at o+0x214 by 2. Matches the sibling pattern in
 * src/overlays/sc01_0077/8017df5c.c (o->f198 = func_8013767C(o->f198)). */
#include "psx_types.h"

extern u32 func_8013767C(u32 a);

void func_800D3274(u8 *o)
{
    u32 p = *(u32 *)(o + 0x198);
    if (p != 0) {
        u32 r = func_8013767C(p);
        u8 b = *(u8 *)(o + 0x214);
        *(u32 *)(o + 0x198) = r;
        *(u8 *)(o + 0x214) = b + 1;
    } else {
        *(u8 *)(o + 0x214) += 2;
    }
}
