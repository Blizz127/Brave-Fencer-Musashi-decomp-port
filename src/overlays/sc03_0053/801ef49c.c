#include "porch.h"

extern u8 D_801F0C34[], D_801F0D24[];

void func_801EF49C(PorchActor *actor)
{
    s32 choice;
    switch (actor->phase) {
    case 0:
        choice = func_801789AC(actor);
        if (choice != 0) {
            if (choice == 1) {
                func_80178CBC(actor, D_801F0C34);
                actor->phase = 1;
            } else {
                func_80178CBC(actor, D_801F0D24);
            }
        }
        break;
    case 1:
        if (func_80178970(actor)) {
            func_80029124(0xec, 1);
            func_80029514(110);
            func_80178D18(actor);
            func_801EF560(actor);
        }
        break;
    }
}
