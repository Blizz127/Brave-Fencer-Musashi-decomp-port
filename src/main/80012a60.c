/* Fold a 12-bit angle difference around the half turn. */
#include "psx_types.h"

s32 func_80012AB0(s32, s32);

s16 func_80012A60(s16 arg0, s16 arg1) {
    s32 delta;

    delta = func_80012AB0(arg0, arg1);
    if ((s16)delta >= 0x800) {
        delta = 0xFFF - delta;
    }
    return (s16)delta;
}
