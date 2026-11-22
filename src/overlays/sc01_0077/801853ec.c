#include "psx_types.h"

typedef void (*ForestEffectStep)(u8 *effect);
extern ForestEffectStep D_801991C4[];

void func_801853EC(u8 *effect)
{
    D_801991C4[*(u16 *)(effect + 2)](effect);
}
