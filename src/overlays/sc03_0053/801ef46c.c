#include "porch.h"

extern u8 D_801F0B24[];

void func_801EF46C(PorchActor *actor)
{
    actor->action = 15;
    actor->phase = 0;
    func_80178CBC(actor, D_801F0B24);
}
