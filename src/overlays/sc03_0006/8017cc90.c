#include "bincho_burst.h"

void func_8017CC90(void *slot)
{
    BinchoBurstActor *actor = slot;

    D_80190104[actor->phase](actor);
}
