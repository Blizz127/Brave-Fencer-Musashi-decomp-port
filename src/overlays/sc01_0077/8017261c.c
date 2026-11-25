#include "forest_actor_mask.h"

u32 func_8017261C(ForestPlayerState *actor)
{
    actor->status_mask |= 0x80u;
    return actor->status_mask;
}
