/* Owner re-derivation from retail assembly (clean input set). */
/* Calls func_8012F14C with the address 0x34 into the sub-object at offset 0x20. */
#include "psx_types.h"

extern void func_8012F14C(void *p);

void func_80149350(u8 *obj) {
    func_8012F14C(*(u8 **)(obj + 0x20) + 0x34);
}
