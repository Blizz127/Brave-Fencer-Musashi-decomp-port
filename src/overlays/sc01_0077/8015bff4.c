/* Kind four selects the current action using its unsigned halfword. */
#include "forest_player_state.h"

#if defined(__mips__) && !defined(__LP64__)
extern u32 D_80189338[];
#endif

s32 func_8015BFF4(ForestPlayerState *actor)
{
    return FOREST_ACTION_RESULT(80189338, actor->action, actor);
}
