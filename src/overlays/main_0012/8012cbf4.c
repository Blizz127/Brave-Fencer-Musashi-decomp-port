/* Owner re-derivation from retail assembly (clean input set). */
/* Calls func_8012CC88 with the object, 0x11 and the table D_800D3918. */
#include "psx_types.h"

extern u8 D_800D3918[];
extern void func_8012CC88(void *obj, s32 id, void *table);

void func_8012CBF4(void *obj) {
    func_8012CC88(obj, 0x11, D_800D3918);
}
