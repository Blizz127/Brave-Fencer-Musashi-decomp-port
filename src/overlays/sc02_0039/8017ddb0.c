/* Dispatch the mine rotor state, then synchronize its current model. */
#include "mine_rotor_dispatch.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
typedef void (*RotorAction)(Obj8014A2E4 *actor);
extern RotorAction D_8019B574[];
extern void func_8012B2CC(char *actor);

void func_8017DDB0(Obj8014A2E4 *actor)
{
    D_8019B574[*(u16 *)actor->pad002](actor);
    func_8012B2CC((char *)actor);
}
#else
int mine_rotor_dispatch(Obj8014A2E4 *actor, const MineRotorActions *actions)
{
    if (!actor || !actions)
        return 0;

    u16 state = (u16)(actor->pad002[0] | (u16)actor->pad002[1] << 8);
    if (state > 1)
        return 0;
    MineRotorAction selected = state ? actions->update : actions->initial;
    if (!selected || !selected(actions->context, actor))
        return 0;
    return actions->synchronize && actions->synchronize(actions->context, actor);
}
#endif
