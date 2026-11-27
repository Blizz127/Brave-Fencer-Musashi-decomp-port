/* Replace the active target with the supplied foot and mark it available. */
#include "forest_foot_target.h"

void func_8014C8C8(ForestTargetOwner *actor, const ForestTarget *foot)
{
    actor->target = *foot;
    actor->target.status = 0;
}
