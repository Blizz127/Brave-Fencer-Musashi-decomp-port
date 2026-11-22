#include "mine_actor_motion.h"

u32 func_80147460(MineActorMotion *actor)
{
    actor->position[1] += actor->vertical_step;
    return actor->position[1];
}
