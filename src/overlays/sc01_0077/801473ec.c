/* Advance the player's three fixed-point positions by their current velocities. */
#include "../../../include/forest_action.h"

s32 func_801473EC(ForestActionPlayer *actor)
{
    u32 result;
    actor->position[0] += actor->velocity[0];
    actor->position[1] += actor->velocity[1];
    actor->position[2] += actor->velocity[2];
    if (actor->velocity[0] || actor->velocity[1] || (result = actor->velocity[2]) != 0) {
        u32 flags = actor->flags | 0x80000000u;
        actor->flags = flags;
        result = flags;
    }
    return result <= 0x7fffffffu ? (s32)result : -1 - (s32)~result;
}
