/* Report whether the collision flags left this action available. */
#include "../../../include/forest_player_state.h"

s32 func_80161104(ForestPlayerState *actor)
{
    return func_80161208(actor) == 0;
}
