/* Refresh the active target from the whole coordinates of the player. */
#include "forest_foot_target.h"

void func_80015978(u16 *fixed_position, u16 *whole_position);

void func_8014C88C(ForestTargetOwner *actor)
{
    ForestTarget foot;

    func_80015978(actor->position_halves, foot.position);
    func_8014C8C8(actor, &foot);
}
