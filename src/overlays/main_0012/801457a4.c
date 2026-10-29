/* Owner re-derivation from retail assembly (clean input set). */
/* Looks up the handler word for a scene/event id: ids in the listed set (0x1010, 0x1052, 0x1077,
 * 0x1094, 0x2013 and the runs 0x2015-0x201A, 0x2055-0x205B, 0x207A-0x2080, 0x2097-0x209D) return
 * D_8017E338[id with bits 12-15 cleared]; every other id returns entry 0. */
#include "psx_types.h"

extern u32 D_8017E338[];

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
        return D_8017E338[id & 0xFFFF0FFF];
    default:
        return D_8017E338[0];
        break;
    }
}
