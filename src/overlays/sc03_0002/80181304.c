#include "macho_gate.h"

void func_80181304(MachoActor *actor)
{
    if (func_80183550(actor)) {
        actor->phase = 2;
        func_8012A828(actor, D_801BD368);
        func_8012E88C(actor);
        func_8012E8E0(actor, D_80189850);
        actor->contact_shape = (u32)D_80189874 | 0x40000000;
        actor->facing = 0x800;
    }
}
