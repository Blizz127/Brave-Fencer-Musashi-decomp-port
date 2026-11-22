#include "porch.h"

extern u16 D_801F0900[];
extern u8 D_801F0934[], D_801F0A6C[], D_801F0A9C[];

void func_801EF734(PorchActor *actor)
{
    actor->action = 5;
    func_8012E8E0((u8 *)actor, D_801F0900);
    func_8012E88C(actor);
    func_80187344(actor);
    if ((u8)func_80029178(0xDF)) {
        actor->phase = 0;
        func_80178CBC(actor, D_801F0A6C);
    } else if ((u8)func_80029178(0xE0)) {
        actor->phase = 1;
        func_80178CBC(actor, D_801F0A9C);
    } else {
        actor->phase = 2;
        func_80178CBC(actor, D_801F0934);
    }
}
