/* Test the player's longitudinal position and distance for a body action. */
#include "steam_knight_action_search.h"

extern s32 func_80132E6C(SteamKnightPart *body);
#if defined(__mips__) && !defined(__LP64__)
extern s16 D_80126CB8;
#define PLAYER_Z D_80126CB8
#else
#define PLAYER_Z ((s16)bfm_action_half(bfm_tower_spawn_pointer(0x80126cb8u), 0))
#endif

s32 func_801860F4(SteamKnightPart *body, s16 action)
{
    s32 distance = func_80132E6C(body);
    (void)action;
    if (PLAYER_Z <= (s16)SLOT_Z(body))
        return distance >= 25600;
    return 0;
}
