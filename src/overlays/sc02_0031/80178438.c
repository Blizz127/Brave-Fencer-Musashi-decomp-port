/* Owner re-derivation from retail assembly (clean input set). */
/* func_8001534C(2, &D_801B5FA0, 0x80, 0x88, 0, 0); func_800153CC(3, o->0 (u16), 0xA8, 0x88,
 * 0x40, 0). The second call uses an unprototyped extern so each arg keeps its own natural
 * integer promotion (matching the established pattern in src/overlays/sc02_0035/80136dfc.c). */
#include "psx_types.h"

extern void func_8001534C(s16, void *, int, s16, int, int);
extern s32 D_801B5FA0;
extern void func_800153CC(); /* no prototype in scope: args keep their own-type promotion */

void func_80178438(u8 *o)
{
    func_8001534C(2, &D_801B5FA0, 0x80, 0x88, 0, 0);
    func_800153CC(3, *(u16 *)(o + 0), 0xA8, 0x88, 0x40, 0);
}
