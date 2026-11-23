#include "action22_poll.h"

void func_8015FDCC(Action21Actor *base)
{
    TwinpeakPollActor *actor = (TwinpeakPollActor *)base;

    if (func_801399F0(actor->handle)) {
        func_80147324(25);
        func_80139914(actor->handle);
        actor->handle = 0;
        if (!func_80028DE0() && (unsigned char)func_80165700(base, actor->contact)) {
            func_801466F0(36, TWINPEAK_ACTOR_WORD(base), 0, 0, 0, 0, 0, 0);
            func_80147324(2441);
        }
        func_80146CA0(base);
    }
}
