#include "macho_gate.h"

void func_8018137C(MachoActor *actor)
{
    if (func_80183550(actor)) {
        actor->phase = 1;
        actor->substate = 0;
        func_8012E8A8(actor);
        func_8012E8E0(actor, D_80189850);
        actor->contact_shape = (u32)D_80189874 | 0x40000000;
        actor->facing = 0;
    }
}
