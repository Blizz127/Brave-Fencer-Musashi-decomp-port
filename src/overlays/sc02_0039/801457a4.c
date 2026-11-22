/* Select a room descriptor word from the mine member. Unlisted scene IDs use entry zero. */
#include "psx_types.h"

extern u32 D_8017F0B0[];

u32 func_801457A4(s32 id)
{
    switch (id) {
    case 0x1010:
    case 0x1052:
    case 0x1077:
    case 0x1094:
    case 0x2013:
    case 0x2015 ... 0x201A:
    case 0x2055 ... 0x205B:
    case 0x207A ... 0x2080:
    case 0x2097 ... 0x209D:
        return D_8017F0B0[id & 0xFFFF0FFF];
    default:
        return D_8017F0B0[0];
        break;
    }
}
