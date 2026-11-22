#include "actor_position_updates.h"

extern void func_80178840(Obj8014A2E4 *actor);

void func_801787D4(void)
{
    s32 index;
    Obj8014A2E4 *actor = D_801202A0;

    for (index = 0; index < 96; index++, actor++) {
        if (actor->active && ((const u8 *)actor)[0xc0]) {
            func_80178840(actor);
        }
    }
}
