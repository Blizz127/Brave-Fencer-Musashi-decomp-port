/* Start phase 19 of the Steam Knight introduction when the player timer expires. */
#include "tower_intro_player.h"

extern s32 D_8012695C;
extern s16 D_80126968, D_80126978;

u32 func_8017FF8C(TowerIntroPlayer *player)
{
    TOWER_INTRO_RESULT(result) = --player->timer;
    if (result == (u32)-1) {
        /* Install the new timer before writing the three transition parameters. */
        volatile u32 *timer = &player->timer;
        *timer = 64;
        D_8012695C = 400;
        D_80126968 = -341;
        D_80126978 = 32;
        D_801270C8 = 19;
        result = (u32)player->step + 1;
        player->step = (u8)result;
    }
    return result;
}
