/* Fresh retail-only derivation: carry the actual tested value through V0. */
#include "psx_types.h"

s32 func_80149CD4(u8 *object)
{
#if defined(__mips__) && __GNUC__ == 2
    register s32 result __asm__("$2") = *(u32 *)(object + 0x44) & 0x404;
#else
    s32 result = *(u32 *)(object + 0x44) & 0x404;
#endif
    if (result) {
        result = 0;
    } else {
        result = *(s16 *)(object + 0x1C8);
        if (result) {
            result = 0;
        } else {
            result = *(u16 *)(object + 0xAA) & 8;
            result = (u32)result != 0;
        }
    }
    return result;
}
