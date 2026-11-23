#include "jon_stocks.h"

void func_801EF558(JonStocksActor *actor)
{
    int progress;

    actor->state = 6;
    D_801F1480 = 0;
    D_801F1482 = 0;
    progress = func_80029504();
    if (progress < 40) {
        func_8002D4C8(18, 0);
        func_80178CBC(actor, D_801F10C0);
        func_80029514(40);
        func_80029124(227, 1);
        actor->branch = 0;
    } else if (progress < 50) {
        func_8002D4C8(18, 0);
        func_80178CBC(actor, D_801F10E0);
        actor->branch = 1;
    } else if ((unsigned char)func_80029178(228)) {
        func_8002D4C8(18, 0);
        func_80178CBC(actor, D_801F10F8);
        actor->branch = 5;
    } else if (progress < 60) {
        func_8002D4C8(18, 0);
        func_80178CBC(actor, D_801F1118);
        actor->branch = 2;
    } else if (progress < 70) {
        if (func_8018766C(0, 5)) {
            func_8002D4C8(18, 0);
            func_80178CBC(actor, D_801F1158);
            func_800D0F4C(77);
            actor->branch = 6;
        } else {
            func_8002D4C8(18, 0);
            func_80178CBC(actor, D_801F1138);
            actor->branch = 2;
        }
    }
}
