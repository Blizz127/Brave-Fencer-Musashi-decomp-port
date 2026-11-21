/* Fresh retail-only derivation: kind-dependent input-state query. */
#include "psx_types.h"

s32 func_80148A48(u8 *object)
{
    switch (object[0xA9]) {
    case 0x41:
        return *(u16 *)(object + 0xAA) & 0x2000;
    case 0x53:
    case 0x73:
        return object[0xAE] >= 0xC1;
    default:
        return 0;
    }
}
