/* Keep the supplied foot through the next player-position refresh. */
#include "forest_foot_target.h"

void func_8014C860(ForestTargetOwner *actor, const ForestTarget *foot)
{
    actor->supplied_foot = *foot;
    actor->supplied_foot.status = 1;
}
