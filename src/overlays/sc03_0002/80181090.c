#include "macho_gate.h"

void func_80181090(MachoActor *actor)
{
    s32 talks = func_800291B4(0xdc);

    func_80029124(0xde, 1);
    if (talks == 0)
        actor->message = D_801A9EE0;
    else if (talks & 1)
        actor->message = D_801A9F5C;
    else
        actor->message = D_801A9FFC;
    if ((u8)++talks == 0)
        talks = 2;
    func_800291A0(0xdc, (u8)talks);
}
