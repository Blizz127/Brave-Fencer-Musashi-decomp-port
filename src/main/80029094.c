/* Owner re-derivation from retail assembly (clean input set). */
/* Calls func_80016714 on a fixed buffer with size 0x60. */
#include "psx_types.h"

extern u8 D_800BA2B8[];
extern void func_80016714(void *, s32);

void func_80029094(void) {
    func_80016714(D_800BA2B8, 0x60);
}
