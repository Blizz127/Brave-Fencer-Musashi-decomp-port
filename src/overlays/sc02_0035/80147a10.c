/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern void func_80147A30(void *o, s32 delta, s32 limit);

void func_80147A10(void *o, s32 delta) {
    func_80147A30(o, delta, 0x1F0000);
}
