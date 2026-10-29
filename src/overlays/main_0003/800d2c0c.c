/* Owner re-derivation from retail assembly (clean input set). */
/* Sets a primitive's texture-disable (flag bit 0 clear) and semi-transparency (flag bit 1) code bits, then hands it to func_800D2E64. */
#include "psx_types.h"

extern void func_800D2E64(u32 *, u16);

void func_800D2C0C(u32 *prim, s32 flags) {
    if (flags & 1) {
        *prim &= ~0x08000000;
    } else {
        *prim |= 0x08000000;
    }
    if (flags & 2) {
        *prim |= 0x02000000;
    } else {
        *prim &= ~0x02000000;
    }
    func_800D2E64(prim, *(u16 *)prim);
}
