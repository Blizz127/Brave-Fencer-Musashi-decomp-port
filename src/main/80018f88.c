/* Reset the current input report when the controller is disconnected or searching. */
#include "controller_report.h"

#ifdef __mips__
typedef unsigned long Address;
#else
#include <stdint.h>
typedef uintptr_t Address;
#endif

enum {
    CURRENT_REPORT = 0x2A,
    BUTTON_BYTES = 2,
    AXIS_BYTES = 4
};

void func_80018F88(PadRecord *record)
{
    s32 index = BUTTON_BYTES - 1;
    /* The final decrement may step past the beginning of the record. */
    Address cursor = (Address)(u8 *)record + index;

    do {
        ((u8 *)cursor)[CURRENT_REPORT] = 0;
        --cursor;
    } while (--index >= 0);

    for (index = BUTTON_BYTES; index < BUTTON_BYTES + AXIS_BYTES; ++index) {
        record->current[index] = 0x80;
    }
}
