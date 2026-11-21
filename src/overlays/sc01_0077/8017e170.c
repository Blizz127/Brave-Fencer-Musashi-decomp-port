#include "player_direction.h"

s32 func_8017E170(ScenePlayer *player)
{
    s32 result = func_80171B4C(player, 30);
    if (result != 0) {
        result = player->event_state + 1;
        player->event_state = (u8)result;
    }
    return result;
}
