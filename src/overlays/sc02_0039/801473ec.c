#include "mine_actor_motion.h"

u32 func_801473EC(MineActorMotion *actor)
{
    u32 result;

    actor->position[0] += actor->velocity[0];
    actor->position[1] += actor->velocity[1];
    actor->position[2] += actor->velocity[2];
    if (actor->velocity[0] || actor->velocity[1] || (result = actor->velocity[2])) {
        u32 flags = actor->flags | 0x80000000u;
        actor->flags = flags;
        result = flags;
    }
    return result;
}
