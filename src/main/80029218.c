/* Owner re-derivation from retail assembly (clean input set). */
/* Calls func_80016714 on global D_800AE6A8 with count 0x3C. */
#include "psx_types.h"

extern u8 D_800AE6A8[];
extern void func_80016714(void *p, s32 n);

void func_80029218(void) {
    func_80016714(D_800AE6A8, 0x3C);
}
