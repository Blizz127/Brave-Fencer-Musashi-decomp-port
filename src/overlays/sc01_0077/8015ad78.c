/* The action-zero entry advances the player only when the contact check allows it. */
#include "../../../include/forest_player_state.h"

s32 func_8015AD78(ForestPlayerState *actor)
{
    s32 result = func_80161104(actor);
    if (result)
        result = func_8015ADB0(actor);
    return result;
}
