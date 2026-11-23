#include "handle.h"

TutorialHandleSlot *func_8013767C(unsigned int descriptor)
{
    TutorialHandleSlot *slot = func_801399A8();

    if (slot != 0) {
        func_801377B4(descriptor, 0, slot);
    }
    return slot;
}
