/* Arm the short delay before the first acquisition pose. */
#include "scroll_actor.h"

void func_801800C4(ScrollActor *actor)
{
    actor->wait = 1;
    func_80171A1C(actor);
}
