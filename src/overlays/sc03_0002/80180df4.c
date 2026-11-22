#include "macho_gate.h"

void func_80180DF4(MachoActor *actor)
{
    actor->phase = 5;
    func_8012E8E0(actor, D_80189904);
    func_8012E88C(actor);
    func_8012A828(actor, D_801BD8B0);
    actor->contact_shape = (u32)D_80189884 | 0x40000000;
    actor->movement_timer = 0;
    actor->facing = 0x800;
    actor->movement = 0;
    func_80183480(actor, D_801BDC00, 1, 96);
    func_8012B030(actor);
    func_8012B2CC(actor);
}
