#include "mine_child_spawn.h"

void func_8017E640(MineEmitterActor *actor)
{
    u8 subtype;

    if (actor->subtype == 0)
        func_8017E794(actor);
    if (func_8012BEE8(actor)) {
        MineEmitterStep *sequence = D_8019B594;
        if (sequence[actor->index].subtype == 0xff)
            actor->index = 0;
        subtype = sequence[actor->index].subtype;
        if (subtype & 4) {
            subtype = func_8005C49C() & 1;
            if (subtype)
                subtype |= 0x40;
            else
                subtype |= 0x80;
        }
        actor->timer = D_8019B594[actor->index].delay;
        if ((u16)(actor->range + 255) < 511 && actor->distance < 336)
            func_8012C658(0x1a9, subtype, actor);
        actor->index++;
    }
}
