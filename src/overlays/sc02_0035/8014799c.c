/* Owner re-derivation from retail assembly (clean input set). */
/* Calls func_801479BC with a clamp limit of 0x1F0000. */
#include "psx_types.h"

extern void func_801479BC(void *obj, s32 delta, s32 limit);

void func_8014799C(void *obj, s32 delta) {
    func_801479BC(obj, delta, 0x1F0000);
}
