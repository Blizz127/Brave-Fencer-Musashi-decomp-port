/* Owner re-derivation from retail assembly (clean input set). */
/* Gets a vector from func_80147CC8 into the scratch words D_80127090..98, then
 * moves the scratch vector to o+0x10 and clears it (func_80146D30's body).
 * The scratch is written through a 3-word struct view and read back as the
 * scalars; gcc 2.7.2 CSE does not match those addresses, so retail reloads. */
#include "psx_types.h"

typedef struct { s32 x, y, z; } V3;

extern s32 D_80127090;
extern s32 D_80127094, D_80127098;
extern void func_80147CC8(void *, s32, s32, s32, void *);

/* Callers do not read v0. */
void func_80147C30(u8 *o, s32 a, s32 b, s32 c) {
    V3 *p;
    s32 t[3];
    p = (V3 *)&D_80127090;
    func_80147CC8(o, a, b, c, t);
    p->x = t[0];
    p->y = t[1];
    p->z = t[2];
    *(s32 *)(o + 0x10) = D_80127090;
    *(s32 *)(o + 0x14) = D_80127094;
    *(s32 *)(o + 0x18) = D_80127098;
    D_80127090 = 0;
    D_80127094 = 0;
    D_80127098 = 0;
}
