/* Owner re-derivation from retail assembly (clean input set). */
/* Adds delta to the signed 32-bit value at offset 0x24 of the object and clamps it to [-limit, limit]. */
#include "psx_types.h"

void func_80147948(u8 *obj, s32 delta, s32 limit) {
    s32 v = *(s32 *)(obj + 0x24) + delta;
    *(s32 *)(obj + 0x24) = v;
    if (delta < 0 && v <= -limit) {
        *(s32 *)(obj + 0x24) = -limit;
        return;
    }
    if (delta > 0 && *(s32 *)(obj + 0x24) >= limit) {
        *(s32 *)(obj + 0x24) = limit;
    }
}
