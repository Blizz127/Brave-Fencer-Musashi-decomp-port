/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
void func_8012E4C8(u8 *o) {
    u8 *p = *(u8 **)(o + 0x20);
    u16 t;
    *(u16 *)(p + 0x1C) = 0x1000;
    *(u16 *)(p + 0x18) = 0x1000;
    *(u16 *)(o + 0x5C) |= 1;
    *(u16 *)(o + 0x60) = 0;
    t = *(u16 *)(*(u8 **)(o + 0x20) + 0x12);
    *(u16 *)(o + 0x5E) = 1;
    *(u16 *)(o + 0x62) = t + 0x800;
}
