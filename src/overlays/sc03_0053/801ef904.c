#include "porch.h"

extern u16 D_801F0900[];
extern u8 D_801F0ACC[];

void func_801EF904(PorchActor *actor)
{
    actor->action = 10;
    func_8012E8E0((u8 *)actor, D_801F0900);
    func_8012E88C(actor);
    func_80187344(actor);
    func_80178CBC(actor, D_801F0ACC);
}
