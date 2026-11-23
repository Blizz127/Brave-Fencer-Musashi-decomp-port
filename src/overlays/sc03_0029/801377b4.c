#include "handle.h"

void func_801377B4(unsigned int descriptor, int flags, TutorialHandleSlot *slot)
{
    func_80137840(slot);
    slot->state = 1;
    slot->descriptor = descriptor;
    slot->flags = (short)flags;

    if (!(flags & 0x2000)) {
        TutorialHandleSlot **current = &D_80127524;

        if (*current != 0) {
            D_80127528 = *current;
            (*current)->active_state = 2;
        }
        *current = slot;
    }
}
