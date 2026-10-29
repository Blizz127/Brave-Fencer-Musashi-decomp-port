/* Owner re-derivation from retail assembly (clean input set). */
/* Byte-swaps a 32-bit value (big-endian file word to host order). */
#include "psx_types.h"

u32 func_8003A3D8(u32 x) {
    return (x << 24) + (((x >> 8) & 0xFF) << 16) + ((x & 0xFF0000) >> 8) | (x >> 24);
}
