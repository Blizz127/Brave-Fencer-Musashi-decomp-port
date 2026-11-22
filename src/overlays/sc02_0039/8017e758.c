#include "mine_emitter_stages.h"

void func_8017E758(MineActorStage *actor)
{
#if !defined(__mips__) || defined(__LP64__)
    /* Only the two installed room stages have host callbacks. */
    if (!actor || actor->stage >= 2 || !D_8019B5A4[actor->stage])
        return;
#endif
    D_8019B5A4[actor->stage](actor);
}
