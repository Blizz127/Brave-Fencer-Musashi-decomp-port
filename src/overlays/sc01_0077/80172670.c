#include "forest_actor_mask.h"

u32 func_80172670(const ForestPlayerState *actor)
{
    return actor->status_mask & 0x4u;
}
