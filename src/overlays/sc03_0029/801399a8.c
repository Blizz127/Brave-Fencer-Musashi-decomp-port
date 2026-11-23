#include "handle.h"

TutorialHandleSlot *func_801399A8(void)
{
    int i;

    for (i = 1; i < 3; i++) {
        if (D_801269F0[i].descriptor == 0) {
            return &D_801269F0[i];
        }
    }
    return 0;
}
