#include "tower_handover.h"

u32 func_80180328(TowerIntroPlayer *player)
{
    func_80175414();
    D_801270C8 = 22;
    HANDOVER_SET_HALF(D_800B9AAC, 0, 0x7fff);
    func_8014C6AC();
    func_8016F0E4((u8 *)player);
    return HANDOVER_LAST(player);
}
