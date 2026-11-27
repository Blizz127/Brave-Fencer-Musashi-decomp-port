#include "psx_types.h"

typedef void (*ActorState)(u8 *actor);
extern ActorState D_8018B000[3];

void func_80184908(u8 *actor)
{
    u16 state = *(u16 *)(actor + 2);
    D_8018B000[state](actor);
}
