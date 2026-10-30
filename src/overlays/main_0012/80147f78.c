/* Owner re-derivation from retail assembly (clean input set). */
/* Applies func_80147928/8014799C/80147A10 with a, b, c, copies the vector at
 * o+0x24 into the scratch words D_80127090..98, then moves the scratch vector to
 * o+0x10 and clears it. The scratch is written through a 3-word struct view and
 * read back as the scalars, which keeps retail's reloads. */
#include "psx_types.h"

typedef struct { s32 x, y, z; } V3;

extern s32 D_80127090;
extern s32 D_80127094, D_80127098;
extern void func_80147928(void *o, s32 a);
extern void func_8014799C(void *o, s32 a);
extern void func_80147A10(void *o, s32 a);

/* Callers do not read v0. */
void func_80147F78(u8 *o, s32 a, s32 b, s32 c) {
    s32 unused[16]; /* retail frame reserves 0x40 bytes that no path touches */
    V3 *p;
    p = (V3 *)&D_80127090;
    func_80147928(o, a);
    func_8014799C(o, b);
    func_80147A10(o, c);
    p->x = *(s32 *)(o + 0x24);
    p->y = *(s32 *)(o + 0x28);
    p->z = *(s32 *)(o + 0x2C);
    *(s32 *)(o + 0x10) = D_80127090;
    *(s32 *)(o + 0x14) = D_80127094;
    *(s32 *)(o + 0x18) = D_80127098;
    D_80127090 = 0;
    D_80127094 = 0;
    D_80127098 = 0;
}
