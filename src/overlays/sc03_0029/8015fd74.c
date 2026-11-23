#include "action22_contact.h"

void func_8015FD74(Action21Actor *base)
{
    TwinpeakPollActor *actor = (TwinpeakPollActor *)base;

    func_80147324(24);
    actor->handle = func_8013767C(D_8018A208[actor->contact]);
    func_80146CA0(base);
}
