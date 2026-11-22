#include "porch.h"

void func_801EF6B0(PorchActor *actor)
{
    switch (actor->phase) {
    case 0:
        if (func_80178970(actor)) {
            func_80029124(0xEF, 1);
            func_80178D18(actor);
            func_801858F8(actor);
        }
        break;
    case 1:
        if (func_80178970(actor)) {
            func_80178D18(actor);
            func_801858F8(actor);
        }
        break;
    }
}
