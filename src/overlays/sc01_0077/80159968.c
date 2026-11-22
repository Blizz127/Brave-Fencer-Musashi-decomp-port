/* The current phase can be changed independently of the outer state. */
#include "../../../include/forest_player_state.h"

void func_80159968(ForestPlayerState *actor)
{
    FOREST_PLAYER_CALL(801891AC, actor->phase, actor);
}
