#include "forest_player_state.h"

#if defined(__mips__) && !defined(__LP64__)
extern u32 D_8018A79C[];
#endif

void func_8017E868(ForestPlayerState *player)
{
    FOREST_PLAYER_CALL(8018A79C, player->scene_stage, player);
}
