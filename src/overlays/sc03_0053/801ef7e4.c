#include "porch.h"

extern u8 D_801F09D4[], D_801F099C[], D_801F0A3C[], D_801F0A0C[];

void func_801EF7E4(PorchActor *actor)
{
    s32 choice;

    switch (actor->phase) {
    case 0:
    case 3:
        if (func_80178970(actor)) {
            func_80178D18(actor);
            func_80185874(actor);
        }
        break;
    case 1:
    case 2:
        choice = func_801789AC(actor);
        if (choice == 0)
            break;

        if (choice == 1) {
            func_80029124(0xDF, 1);
            if (actor->phase == 1)
                func_80178CBC(actor, D_801F09D4);
            else
                func_80178CBC(actor, D_801F099C);
        } else {
            func_80029124(0xE0, 1);
            if (actor->phase == 1)
                func_80178CBC(actor, D_801F0A3C);
            else
                func_80178CBC(actor, D_801F0A0C);
        }
        actor->phase = 3;
        break;
    }
}
