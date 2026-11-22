#include "tower_handover.h"

u32 func_801511A8(u8 *player)
{
    u32 flags = HANDOVER_HALF(player, 0x3c);
    HANDOVER_SET_HALF(player, 0x3e, 0);
    HANDOVER_SET_HALF(player, 0x40, 0);
    HANDOVER_SET_HALF(player, 0x42, 0);
    flags &= 0xfffe;
    HANDOVER_SET_HALF(player, 0x3c, flags);
    return flags;
}
