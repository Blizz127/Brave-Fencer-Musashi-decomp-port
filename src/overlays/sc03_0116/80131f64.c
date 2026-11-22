#include "door_15d.h"

typedef s32 (*Door15DPhase)(Door15DActor *);

extern Door15DPhase D_80182E28[];

s32 func_80131F64(Door15DActor *door)
{
    return D_80182E28[door->phase](door);
}
