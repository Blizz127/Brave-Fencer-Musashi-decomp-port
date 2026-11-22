#include "forest_player_state.h"

extern u32 func_80172664(u8 *player);

void func_8017E974(ForestPlayerState *player)
{
    if (func_80172664((u8 *)player)) {
        player->field_aa = player->field_ac = 0xc040;
    } else {
        ++player->scene_stage;
    }
}
