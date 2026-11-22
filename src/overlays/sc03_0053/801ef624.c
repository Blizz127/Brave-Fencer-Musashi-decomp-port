#include "porch.h"

extern u16 D_801F0900[];
extern u8 D_801F0DD0[], D_801F0E10[];

void func_801EF624(PorchActor *actor)
{
    actor->action = 26;
    func_8012E8E0((u8 *)actor, D_801F0900);
    func_8012E88C(actor);
    func_80187344(actor);
    if ((u8)func_80029178(0xEF)) {
        func_80178CBC(actor, D_801F0E10);
        actor->phase = 1;
    } else {
        func_80178CBC(actor, D_801F0DD0);
        actor->phase = 0;
    }
}
