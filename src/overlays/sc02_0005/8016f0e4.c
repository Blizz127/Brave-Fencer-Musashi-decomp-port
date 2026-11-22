#include "tower_handover.h"

s32 func_8016F0E4(u8 *player)
{
    HANDOVER_ZERO_WORD(player, 0x1f8);
    HANDOVER_ZERO_WORD(player, 0x1fc);
    func_8017196C(player, 0);
    func_80147060(player);
    func_801511A8(player);
    if (D_80078EB4)
        HANDOVER_LIVE_CONTROL = 1;
    return func_80148634(player);
}
