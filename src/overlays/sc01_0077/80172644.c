#include "forest_actor_mask.h"

u32 func_80172644(ForestPlayerState *actor)
{
    actor->status_mask |= 0x200u;
    return actor->status_mask;
}
