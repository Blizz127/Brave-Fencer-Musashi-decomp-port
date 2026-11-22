#include "macho_gate.h"

void func_80180F1C(MachoActor *actor)
{
    if (func_80029178(0xf0) || (u32)(func_80029504() - 133) < 2) {
        func_8012CAE4(actor);
        return;
    }

    if (func_80182D68(actor, D_80189860)) {
        actor->contact->extent = 0x7fff;
        actor->contact_shape = (u32)D_80189874 | 0x40000000;
        actor->facing = 0x800;
        if (func_80183158(9, 18)) {
            actor->phase = 2;
            func_8012A828(actor, D_801BD368);
            func_8012E88C(actor);
            func_8012E8E0(actor, D_80189850);
            actor->contact_shape = (u32)D_80189874 | 0x40000000;
            actor->facing = 0x800;
        } else {
            actor->phase = 1;
            actor->substate = 0;
            func_8012E8A8(actor);
            func_8012E8E0(actor, D_80189850);
            actor->contact_shape = (u32)D_80189874 | 0x40000000;
            actor->facing = 0;
        }
        actor->interaction = func_801788B8(actor, func_8018118C);
    }
}
