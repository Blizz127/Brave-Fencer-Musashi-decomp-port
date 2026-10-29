/* Owner re-derivation from retail assembly (clean input set). */
/* Forwards the record at +0x20 plus 0x34, and its two own pass-through arguments a1 and a2, to func_8012F14C. */
#include "psx_types.h"

extern void func_8012F14C(void *p, s32 a1, s32 a2);

void func_80149350(u8 *obj, s32 a1, s32 a2) {
    func_8012F14C(*(u8 **)(obj + 0x20) + 0x34, a1, a2);
}
