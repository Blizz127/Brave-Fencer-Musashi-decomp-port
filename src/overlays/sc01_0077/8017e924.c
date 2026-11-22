#include "forest_player_state.h"
#include "player_direction.h"

void func_8017E924(ForestPlayerState *player)
{
    if (func_80171B4C((ScenePlayer *)player, 8)) {
        player->scene_countdown = 0x20;
        player->field_aa = player->field_ac = 0xc040;
        ++player->scene_stage;
    }
}
