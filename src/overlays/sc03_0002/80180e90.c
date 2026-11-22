#include "macho_gate.h"

void func_80180E90(MachoActor *actor)
{
    actor->phase = 6;
    func_8012E8E0(actor, D_80189850);
    func_8012E88C(actor);
    func_8012A828(actor, D_801BD8B0);
    actor->contact_shape = (u32)D_80189884 | 0x40000000;
    actor->facing = 0x800;
    actor->movement_timer = 0;
    actor->movement = 0;
    func_80183480(actor, D_801BDC00, 0, 96);
}
