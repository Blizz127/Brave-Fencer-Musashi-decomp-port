#include "forest_actor_mask.h"

void func_8017E974(ForestPlayerState *player)
{
    if (func_80172664(player)) {
        player->field_aa = player->field_ac = 0xc040;
    } else {
        ++player->scene_stage;
    }
}
