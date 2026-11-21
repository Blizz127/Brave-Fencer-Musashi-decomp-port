#include "psx_types.h"

typedef void (*ForestLogStep)(u8 *actor);
extern ForestLogStep D_8018A904[];

/* The caller provides a live object with a valid state in this location. */
void func_8017F6D8(u8 *actor)
{
    D_8018A904[*(u16 *)(actor + 2)](actor);
}
