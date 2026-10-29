/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern void func_80131340(void *);
extern void func_80131CA8(void *, s32);

void func_8012FF00(u8 *o) {
    if (*(u32 *)(o + 0xB4) & 0x200) {
        func_80131340(o);
    }
    func_80131CA8(o, 0x1F);
}
