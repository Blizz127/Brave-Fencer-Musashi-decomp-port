/* Dispatch the mine collision actor using the state following its actor kind. */
#include "mine_collision_dispatch.h"

#if defined(__mips__) && !defined(__LP64__)
extern MineCollisionActorAction D_8019B58C[];
void func_8017DFE8(Obj8014A2E4 *actor)
{
    D_8019B58C[*(u16 *)((u8 *)actor + 2)](actor);
}
#else
int bfm_mine_collision_actor_update(Obj8014A2E4 *actor, const MineCollisionActorActions *actions)
{
    u16 state;
    MineCollisionActorAction action;
    if (!actor || !actions)
        return 0;
    state = (u16)(actor->pad002[0] | (u16)actor->pad002[1] << 8);
    if (state > 1)
        return 0;
    action = state ? actions->update : actions->initial;
    if (!action)
        return 0;
    action(actor);
    return 1;
}
#endif
