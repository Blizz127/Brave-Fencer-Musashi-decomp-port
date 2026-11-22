/* Set or clear one flag. Return the updated byte with all other flags preserved. */
#include "event_flags.h"

extern u8 D_800AE648[];

u32 func_80029124(u32 bit, u32 set) {
    u32 index;
    u32 mask;
    u32 value;

    index = bit >> 3;
    bit &= 7u;
    mask = 1u << bit;

    if ((u8)set) {
        value = D_800AE648[index] | mask;
    } else {
        mask = ~mask;
        value = D_800AE648[index] & mask;
    }
    D_800AE648[index] = (u8)value;
    return value;
}
