/* Owner re-derivation from retail assembly (clean input set). */
/* Sets or clears the top bit of the target word from a flag, copies three halfword fields from the owner record, then calls func_80162968. */
#include "psx_types.h"

extern void func_80162968(void *obj);

void func_801628F4(u8 *obj) {
    u8 *src = *(u8 **)(obj + 4);
    u8 *dst = *(u8 **)(obj + 8);
    if (*(s16 *)(src + 0x7E) != 0) {
        *(u32 *)(dst + 4) |= 0x80000000;
    } else {
        *(u32 *)(dst + 4) &= 0x7FFFFFFF;
    }
    *(s32 *)(obj + 0x20) = *(s16 *)(src + 0x78);
    *(s32 *)(obj + 0x24) = *(s16 *)(src + 0x7A);
    *(s32 *)(obj + 0x28) = *(s16 *)(src + 0x7C);
    func_80162968(obj);
}
