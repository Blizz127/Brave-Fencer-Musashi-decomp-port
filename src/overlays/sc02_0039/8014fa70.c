#include "mine_terrain_action.h"

s32 func_8014FA70(MineTerrainActor *actor)
{
    MineActionPoint from;
    MineActionPoint to;
    s32 result;

    from.x = actor->previous[0];
    from.y = actor->previous[1];
    from.z = actor->previous[2];
    to.x = actor->position[1];
    to.y = actor->position[3];
    to.z = actor->position[5];
    result = func_80133784(1, &from, &to);
    if (result != 0) {
        actor->position[1] = to.x;
        actor->position[3] = to.y;
        actor->position[5] = to.z;
        if (to.surface.action != 0)
            actor->action = to.surface.action;
        actor->surface_class = to.surface.flags >> 13;
        if (actor->action == 29 && (u8)func_80029178(33) == 0) {
            result &= ~0x2000;
            result |= 0x4000;
            __builtin_memcpy(&D_801152B0, &D_801152A8, sizeof(D_801152A8));
            D_801152BC = 1;
        }
        return result;
    }
    if (to.surface.action != 0)
        actor->action = to.surface.action;
    return 0;
}
