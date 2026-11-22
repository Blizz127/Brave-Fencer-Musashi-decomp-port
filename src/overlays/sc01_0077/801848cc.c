#include "psx_types.h"

typedef void (*ForestActorStep)(u8 *actor);
extern ForestActorStep D_8018AFB8[];

void func_801848CC(u8 *actor)
{
    D_8018AFB8[*(u16 *)(actor + 2)](actor);
}
