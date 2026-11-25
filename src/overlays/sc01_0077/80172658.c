#include "forest_actor_mask.h"

u32 func_80172658(const ForestPlayerState *actor)
{
    return actor->status_mask & 0x1u;
}
