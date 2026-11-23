#include "action22_contact.h"

void func_8015FD74(Action21Actor *base)
{
    TwinpeakPollActor *actor = (TwinpeakPollActor *)base;

    func_80147324(24);
    actor->handle = twinpeak_contact_handle(D_8018A208[actor->contact]);
    func_80146CA0(base);
}
