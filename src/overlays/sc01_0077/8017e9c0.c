#include "forest_actor_contact.h"
#include "forest_player_state.h"

extern s32 func_80012B04(s16 from, s16 to, s16 divisor);

void func_8017E9C0(ForestPlayerState *player)
{
    s16 change = func_80012B04(*FOREST_ACTOR_REF(s16, player->model + 0x12), 0x800, 4);
    s16 *yaw = FOREST_ACTOR_REF(s16, player->model + 0x12);
    *yaw = (*yaw + change) & 0xfff;
    if (change == 0)
        ++player->scene_stage;
}
