/* Dispatch the gate guard's current actor phase. */
#include "psx_types.h"

typedef struct {
    u16 type;
    u16 phase;
} GuardActor;

extern void (*D_8018962C[])(GuardActor *);

void func_8017FAE8(GuardActor *actor)
{
    D_8018962C[actor->phase](actor);
}
