/* Hold phase 19 for the next countdown, then advance the introduction. */
#include "tower_intro_player.h"

typedef struct {
    u8 bytes[8];
} TowerIntroParameter;

extern TowerIntroParameter D_801E2DE8;

u32 func_8017FFF0(TowerIntroPlayer *player)
{
#if defined(__mips__) && !defined(__LP64__)
    TowerIntroParameter parameter = D_801E2DE8;
#else
    volatile TowerIntroParameter parameter = D_801E2DE8;
#endif
    TOWER_INTRO_RESULT(result) = --player->timer;
    (void)&parameter;
    if (result == (u32)-1) {
        D_801270C8 = 20;
        player->timer = 64;
        result = (u32)player->step + 1;
        player->step = (u8)result;
    }
    return result;
}
