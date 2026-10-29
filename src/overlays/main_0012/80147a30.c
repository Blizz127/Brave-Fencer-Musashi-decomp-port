/* Owner re-derivation from retail assembly (clean input set). */
/* Adds delta to the signed 32-bit value at offset 0x2C of the object and clamps it to [-limit, limit]. */
#include "psx_types.h"

void func_80147A30(u8 *obj, s32 delta, s32 limit) {
    s32 v = *(s32 *)(obj + 0x2C) + delta;
    *(s32 *)(obj + 0x2C) = v;
    if (delta < 0 && v <= -limit) {
        *(s32 *)(obj + 0x2C) = -limit;
        return;
    }
    if (delta > 0 && *(s32 *)(obj + 0x2C) >= limit) {
        *(s32 *)(obj + 0x2C) = limit;
    }
}
