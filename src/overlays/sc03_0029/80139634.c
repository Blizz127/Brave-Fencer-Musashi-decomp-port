#include "message_defaults.h"

void func_80139634(TutorialHandleSlot *slot)
{
    TutorialMessageRect rect;

    __builtin_memcpy(&rect.x, (unsigned char *)slot + 0x38, 2);
    __builtin_memcpy(&rect.y, (unsigned char *)slot + 0x3a, 2);
    rect.width = 56;
    rect.height = 48;
    func_80059888(&rect, 0, 0, 0);
}
