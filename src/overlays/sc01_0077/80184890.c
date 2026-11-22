#include "psx_types.h"

typedef void (*ForestActorStep)(u8 *actor);
extern ForestActorStep D_8018AF90[];

void func_80184890(u8 *actor)
{
    D_8018AF90[*(u16 *)(actor + 2)](actor);
}
