#include "mine_emitter_stages.h"

void func_8017EE9C(MineActorStage *actor)
{
#if !defined(__mips__) || defined(__LP64__)
    /* Only the two installed room stages have host callbacks. */
    if (!actor || actor->stage >= 2 || !D_8019B634[actor->stage])
        return;
#endif
    D_8019B634[actor->stage](actor);
}
