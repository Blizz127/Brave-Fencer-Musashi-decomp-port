#include "porch.h"

extern u16 D_801F0900[];
extern u8 D_801F0D84[];

void func_801EF58C(PorchActor *actor)
{
    actor->action = 21;
    func_8012E8E0((u8 *)actor, D_801F0900);
    func_8012E88C(actor);
    func_80187344(actor);
    func_80178CBC(actor, D_801F0D84);
}
